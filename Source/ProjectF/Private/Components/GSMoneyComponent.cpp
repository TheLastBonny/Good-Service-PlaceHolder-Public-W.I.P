#include "Components/GSMoneyComponent.h"
#include "Core/GSGameState.h"
#include "Items/GSMoneyItem.h"
#include "Core/GSGameplayTags.h"
#include "Attributes/GSMoneyAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/PrimitiveComponent.h"

UGSMoneyComponent::UGSMoneyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bActAsCashRegister = false;
	LocalRegisterMultiplier = 1.0f;
}

void UGSMoneyComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AGSGameState* GSGameState = GetGSGameState())
	{
		GSGameState->OnMoneyChanged.AddDynamic(this, &UGSMoneyComponent::HandleGameStateMoneyChanged);
	}

	if (bActAsCashRegister && GetOwner())
	{
		TArray<UPrimitiveComponent*> PrimitiveComps;
		GetOwner()->GetComponents<UPrimitiveComponent>(PrimitiveComps);
		for (UPrimitiveComponent* PrimComp : PrimitiveComps)
		{
			if (PrimComp)
			{
				PrimComp->OnComponentBeginOverlap.AddDynamic(this, &UGSMoneyComponent::OnOwnerOverlapBegin);
			}
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
	if (AGSGameState* GSGameState = GetGSGameState())
	{
		GSGameState->AddMoneyDirectly(static_cast<float>(Amount));
	}
}

bool UGSMoneyComponent::RemoveMoney(int32 Amount)
{
	if (AGSGameState* GSGameState = GetGSGameState())
	{
		UAbilitySystemComponent* ASC = GSGameState->GetAbilitySystemComponent();
		if (ASC && Amount > 0)
		{
			float CurrentMoney = GSGameState->GetMoney();
			if (CurrentMoney >= Amount)
			{
				ASC->SetNumericAttributeBase(UGSMoneyAttributeSet::GetMoneyAttribute(), CurrentMoney - Amount);
				
				// Broadcast changes locally on the server (clients will receive it via OnRep_Money)
				GSGameState->OnMoneyChanged.Broadcast(FMath::RoundToInt(CurrentMoney - Amount));
				return true;
			}
		}
	}
	return false;
}

void UGSMoneyComponent::HandleGameStateMoneyChanged(int32 NewMoney)
{
	OnMoneyChanged.Broadcast(NewMoney);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Money Deposited! Total Cash: $%d"), NewMoney));
	}
	UE_LOG(LogTemp, Log, TEXT("Money Deposited! Total Cash: $%d"), NewMoney);
}

void UGSMoneyComponent::OnOwnerOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetOwner())
	{
		return;
	}

	// Money processing must occur on the server (authority)
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	if (AGSMoneyItem* MoneyItem = Cast<AGSMoneyItem>(OtherActor))
	{
		// Skip if the money item has already been marked for destruction
		if (!IsValid(MoneyItem))
		{
			return;
		}

		AGSGameState* GSGameState = GetGSGameState();
		if (GSGameState)
		{
			UAbilitySystemComponent* ASC = GSGameState->GetAbilitySystemComponent();
			if (ASC && MoneyItem->MoneyEffectClass)
			{
				float BaseVal = MoneyItem->MoneyValue;
				float AdjustedVal = BaseVal * LocalRegisterMultiplier;

				FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
				EffectContext.AddInstigator(MoneyItem, MoneyItem);
				EffectContext.AddSourceObject(this);

				FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(MoneyItem->MoneyEffectClass, 1.f, EffectContext);
				if (SpecHandle.IsValid())
				{
					// Set magnitude of EarnedMoney via the SetByCaller tag Data.MoneyAmount
					SpecHandle.Data.Get()->SetSetByCallerMagnitude(GSGameplayTags::Data_MoneyAmount, AdjustedVal);
					ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

					// Calculate final added value (including global multiplier) to broadcast
					float GlobalMult = GetGlobalMoneyMultiplier();
					int32 FinalValue = FMath::RoundToInt(AdjustedVal * GlobalMult);

					OnMoneyDeposited.Broadcast(MoneyItem, FinalValue);
				}
			}
		}

		// Destroy the physical drop
		MoneyItem->Destroy();
	}
}
