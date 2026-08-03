#include "Components/GSMoneyComponent.h"
#include "Core/GSGameState.h"
#include "Items/GSMoneyItem.h"
#include "Components/GSMoneyValueComponent.h"
#include "Core/GSGameplayTags.h"
#include "Attributes/GSMoneyAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/PrimitiveComponent.h"

UGSMoneyComponent::UGSMoneyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bActAsCashRegister = false;
	LocalRegisterMultiplier = 1.0f;
	DefaultMoneyEffectClass = nullptr;
	bShowDebugLogs = false;
}

void UGSMoneyComponent::BeginPlay()
{
	Super::BeginPlay();

	FString NetRole = GetOwner() && GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	if (bShowDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("[MONEY_DEBUG][%s] UGSMoneyComponent::BeginPlay initialized on owner %s. ActAsRegister: %d, LocalMultiplier: %f"),
			*NetRole, GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"), bActAsCashRegister, LocalRegisterMultiplier);
	}

	if (AGSGameState* GSGameState = GetGSGameState())
	{
		GSGameState->OnMoneyChanged.AddDynamic(this, &UGSMoneyComponent::HandleGameStateMoneyChanged);
	}

	if (bActAsCashRegister && GetOwner())
	{
		TArray<UPrimitiveComponent*> PrimitiveComps;
		GetOwner()->GetComponents<UPrimitiveComponent>(PrimitiveComps);
		int32 BoundCount = 0;
		for (UPrimitiveComponent* PrimComp : PrimitiveComps)
		{
			if (PrimComp)
			{
				PrimComp->OnComponentBeginOverlap.AddDynamic(this, &UGSMoneyComponent::OnOwnerOverlapBegin);
				BoundCount++;
			}
		}
		if (bShowDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("[MONEY_DEBUG][%s] UGSMoneyComponent::BeginPlay: Bound cash register overlap events to %d primitive components of %s"),
				*NetRole, BoundCount, *GetOwner()->GetName());
		}
	}
}

void UGSMoneyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AGSGameState* GSGameState = GetGSGameState())
	{
		GSGameState->OnMoneyChanged.RemoveDynamic(this, &UGSMoneyComponent::HandleGameStateMoneyChanged);
	}

	Super::EndPlay(EndPlayReason);
}

AGSGameState* UGSMoneyComponent::GetGSGameState() const
{
	if (UWorld* World = GetWorld())
	{
		return Cast<AGSGameState>(World->GetGameState());
	}
	return nullptr;
}

int32 UGSMoneyComponent::GetCurrentMoney() const
{
	if (AGSGameState* GSGameState = GetGSGameState())
	{
		return GSGameState->GetMoney();
	}
	return 0;
}

float UGSMoneyComponent::GetGlobalMoneyMultiplier() const
{
	if (AGSGameState* GSGameState = GetGSGameState())
	{
		return GSGameState->GetMoneyMultiplier();
	}
	return 1.0f;
}

void UGSMoneyComponent::AddMoney(int32 Amount)
{
	if (bShowDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("[MONEY_DEBUG] UGSMoneyComponent::AddMoney: Adding $%d directly"), Amount);
	}
	if (AGSGameState* GSGameState = GetGSGameState())
	{
		GSGameState->AddMoneyDirectly(static_cast<float>(Amount));
	}
}

bool UGSMoneyComponent::RemoveMoney(int32 Amount)
{
	if (bShowDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("[MONEY_DEBUG] UGSMoneyComponent::RemoveMoney: Attempting to remove $%d"), Amount);
	}
	if (AGSGameState* GSGameState = GetGSGameState())
	{
		UAbilitySystemComponent* ASC = GSGameState->GetAbilitySystemComponent();
		if (ASC && Amount > 0)
		{
			float CurrentMoney = GSGameState->GetMoney();
			if (CurrentMoney >= Amount)
			{
				ASC->SetNumericAttributeBase(UGSMoneyAttributeSet::GetMoneyAttribute(), CurrentMoney - Amount);
				
				if (bShowDebugLogs)
				{
					UE_LOG(LogTemp, Warning, TEXT("[MONEY_DEBUG][SERVER] UGSMoneyComponent::RemoveMoney: Successfully removed $%d. Previous: $%f, New: $%f"),
						Amount, CurrentMoney, CurrentMoney - Amount);
				}

				GSGameState->OnMoneyChanged.Broadcast(FMath::RoundToInt(CurrentMoney - Amount));
				return true;
			}
			else
			{
				if (bShowDebugLogs)
				{
					UE_LOG(LogTemp, Warning, TEXT("[MONEY_DEBUG][SERVER] UGSMoneyComponent::RemoveMoney: FAILED. Insufficient funds! Current: $%f, Requested: %d"), CurrentMoney, Amount);
				}
			}
		}
	}
	return false;
}

void UGSMoneyComponent::HandleGameStateMoneyChanged(int32 NewMoney)
{
	OnMoneyChanged.Broadcast(NewMoney);
	if (bShowDebugLogs)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Money Deposited! Total Cash: $%d"), NewMoney));
		}
		UE_LOG(LogTemp, Log, TEXT("[MONEY_DEBUG] HandleGameStateMoneyChanged: Money updated. Total Cash: $%d"), NewMoney);
	}
}

void UGSMoneyComponent::OnOwnerOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetOwner())
	{
		return;
	}

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	if (bShowDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("[MONEY_DEBUG][SERVER] UGSMoneyComponent::OnOwnerOverlapBegin: Register %s overlapped by %s"), 
			*GetOwner()->GetName(), *OtherActor->GetName());
	}

	float BaseVal = 0.0f;
	TSubclassOf<UGameplayEffect> EffectClass = nullptr;
	bool bIsMoneySource = false;

	if (AGSMoneyItem* MoneyItem = Cast<AGSMoneyItem>(OtherActor))
	{
		if (IsValid(MoneyItem))
		{
			BaseVal = MoneyItem->MoneyValue;
			EffectClass = MoneyItem->MoneyEffectClass;
			bIsMoneySource = true;
		}
	}
	else if (IsValid(OtherActor))
	{
		if (UGSMoneyValueComponent* ValueComp = OtherActor->FindComponentByClass<UGSMoneyValueComponent>())
		{
			BaseVal = ValueComp->MoneyValue;
			EffectClass = ValueComp->MoneyEffectClass;
			bIsMoneySource = true;
		}
	}

	if (bIsMoneySource)
	{
		if (!EffectClass)
		{
			EffectClass = DefaultMoneyEffectClass;
			if (bShowDebugLogs)
			{
				UE_LOG(LogTemp, Log, TEXT("[MONEY_DEBUG][SERVER] UGSMoneyComponent::OnOwnerOverlapBegin: Item did not specify MoneyEffectClass, falling back to DefaultMoneyEffectClass: %s"),
					EffectClass ? *EffectClass->GetName() : TEXT("NULL"));
			}
		}

		AGSGameState* GSGameState = GetGSGameState();
		if (GSGameState)
		{
			UAbilitySystemComponent* ASC = GSGameState->GetAbilitySystemComponent();
			if (ASC && EffectClass)
			{
				float AdjustedVal = BaseVal * LocalRegisterMultiplier;

				FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
				EffectContext.AddInstigator(OtherActor, OtherActor);
				EffectContext.AddSourceObject(this);

				FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.f, EffectContext);
				if (SpecHandle.IsValid())
				{
					SpecHandle.Data.Get()->SetSetByCallerMagnitude(GSGameplayTags::Data_MoneyAmount, AdjustedVal);
					ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

					float GlobalMult = GetGlobalMoneyMultiplier();
					int32 FinalValue = FMath::RoundToInt(AdjustedVal * GlobalMult);

					if (bShowDebugLogs)
					{
						UE_LOG(LogTemp, Warning, TEXT("[MONEY_DEBUG][SERVER] UGSMoneyComponent::OnOwnerOverlapBegin: Deposited %s. Base: %f, Adjusted (Local Mult %f): %f, Global Mult: %f, Final Added: $%d"),
							*OtherActor->GetName(), BaseVal, LocalRegisterMultiplier, AdjustedVal, GlobalMult, FinalValue);
					}

					OnMoneyDeposited.Broadcast(OtherActor, FinalValue);
				}
				else
				{
					if (bShowDebugLogs)
					{
						UE_LOG(LogTemp, Error, TEXT("[MONEY_DEBUG][SERVER] UGSMoneyComponent::OnOwnerOverlapBegin: FAILED to create outgoing spec for EffectClass on %s"), *OtherActor->GetName());
					}
				}
			}
			else
			{
				if (bShowDebugLogs)
				{
					UE_LOG(LogTemp, Error, TEXT("[MONEY_DEBUG][SERVER] UGSMoneyComponent::OnOwnerOverlapBegin: ASC (%p) or EffectClass (%p) is NULL!"), ASC, EffectClass.Get());
				}
			}
		}
		else
		{
			if (bShowDebugLogs)
			{
				UE_LOG(LogTemp, Error, TEXT("[MONEY_DEBUG][SERVER] UGSMoneyComponent::OnOwnerOverlapBegin: GSGameState is NULL!"));
			}
		}

		OtherActor->Destroy();
	}
}
