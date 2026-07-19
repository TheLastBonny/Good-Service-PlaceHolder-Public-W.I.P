#include "Machines/GSAbilityUpgradeStation.h"
#include "Components/BoxComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "Core/GSGameState.h"
#include "Items/GSItem.h"
#include "Kismet/GameplayStatics.h"

AGSAbilityUpgradeStation::AGSAbilityUpgradeStation()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	RootComponent = TriggerVolume;
	TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
	TriggerVolume->SetBoxExtent(FVector(100.f, 100.f, 100.f));
}

void AGSAbilityUpgradeStation::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AGSAbilityUpgradeStation::OnOverlapBegin);
		TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AGSAbilityUpgradeStation::OnOverlapEnd);
	}
}

void AGSAbilityUpgradeStation::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	if (APawn* PlayerPawn = Cast<APawn>(OtherActor))
	{
		if (PlayerPawn->Implements<UAbilitySystemInterface>())
		{
			AttemptUpgrade(PlayerPawn);
		}
	}
}

void AGSAbilityUpgradeStation::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// Can be used for UI/effects termination if needed
}

bool AGSAbilityUpgradeStation::CanUpgrade(APawn* PlayerPawn, FText& OutFailReason, FGSAbilityUpgradeLevel& OutNextUpgradeInfo) const
{
	if (!PlayerPawn)
	{
		OutFailReason = FText::FromString(TEXT("Invalid Player Pawn."));
		return false;
	}

	if (!UpgradeData)
	{
		OutFailReason = FText::FromString(TEXT("Upgrade Data Asset not set on machine."));
		return false;
	}

	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(PlayerPawn);
	if (!ASCInterface)
	{
		OutFailReason = FText::FromString(TEXT("Player does not support Ability System."));
		return false;
	}

	UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
	if (!ASC)
	{
		OutFailReason = FText::FromString(TEXT("Player has no active Ability System Component."));
		return false;
	}

	// 1. Find ability spec matching the target gameplay tag
	FGameplayAbilitySpec* FoundSpec = nullptr;
	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const bool bMatch = Spec.Ability && (Spec.Ability->GetAssetTags().HasTag(TargetAbilityTag) || Spec.Ability->AbilityTags.HasTag(TargetAbilityTag));
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		if (bMatch)
		{
			FoundSpec = &Spec;
			break;
		}
	}

	if (!FoundSpec)
	{
		OutFailReason = FText::FromString(FString::Printf(TEXT("Player does not possess the ability with tag: %s"), *TargetAbilityTag.ToString()));
		return false;
	}

	int32 CurrentLevel = FoundSpec->Level;
	int32 TargetLevel = CurrentLevel + 1;

	// 2. Check if we've reached max level
	if (CurrentLevel >= UpgradeData->GetMaxLevel())
	{
		OutFailReason = FText::FromString(TEXT("Max level already reached."));
		return false;
	}

	// 3. Find upgrade level config
	if (!UpgradeData->GetUpgradeForLevel(TargetLevel, OutNextUpgradeInfo))
	{
		OutFailReason = FText::FromString(FString::Printf(TEXT("No upgrade configuration found for Level %d."), TargetLevel));
		return false;
	}

	// 4. Validate costs
	if (OutNextUpgradeInfo.CostType == EGSAUpgradeCostType::Money)
	{
		AGSGameState* GSGameState = Cast<AGSGameState>(GetWorld()->GetGameState());
		if (!GSGameState)
		{
			OutFailReason = FText::FromString(TEXT("Unable to retrieve Game State."));
			return false;
		}

		if (GSGameState->GetMoney() < OutNextUpgradeInfo.MoneyCost)
		{
			OutFailReason = FText::FromString(TEXT("No tienes suficiente dinero."));
			return false;
		}
	}
	else if (OutNextUpgradeInfo.CostType == EGSAUpgradeCostType::Object)
	{
		TArray<AActor*> OverlappingActors;
		TriggerVolume->GetOverlappingActors(OverlappingActors);

		int32 MatchCount = 0;
		for (AActor* Actor : OverlappingActors)
		{
			if (Actor && Actor != PlayerPawn && ActorMatchesTag(Actor, OutNextUpgradeInfo.RequiredObjectTag))
			{
				MatchCount++;
			}
		}

		if (MatchCount < OutNextUpgradeInfo.RequiredQuantity)
		{
			OutFailReason = FText::FromString(FString::Printf(TEXT("Faltan objetos requeridos en el area (%d/%d)."), MatchCount, OutNextUpgradeInfo.RequiredQuantity));
			return false;
		}
	}

	return true;
}

void AGSAbilityUpgradeStation::AttemptUpgrade(APawn* PlayerPawn)
{
	if (!HasAuthority()) return;

	FText FailReason;
	FGSAbilityUpgradeLevel NextUpgrade;

	if (!CanUpgrade(PlayerPawn, FailReason, NextUpgrade))
	{
		if (UpgradeData && PlayerPawn)
		{
			IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(PlayerPawn);
			if (ASCInterface)
			{
				if (UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent())
				{
					for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
					{
						PRAGMA_DISABLE_DEPRECATION_WARNINGS
						const bool bMatch = Spec.Ability && (Spec.Ability->GetAssetTags().HasTag(TargetAbilityTag) || Spec.Ability->AbilityTags.HasTag(TargetAbilityTag));
						PRAGMA_ENABLE_DEPRECATION_WARNINGS
						if (bMatch)
						{
							if (Spec.Level >= UpgradeData->GetMaxLevel())
							{
								UE_LOG(LogTemp, Log, TEXT("UpgradeStation: Player already at max level (%d) for ability %s"), Spec.Level, *TargetAbilityTag.ToString());
								OnMaxLevelReached.Broadcast(PlayerPawn);
								return;
							}
						}
					}
				}
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("UpgradeStation: Upgrade attempt failed for %s. Reason: %s"), 
			PlayerPawn ? *PlayerPawn->GetName() : TEXT("None"), *FailReason.ToString());
		OnUpgradeFailed.Broadcast(PlayerPawn, FailReason);
		return;
	}

	// Double check we have ASC
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(PlayerPawn);
	UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
	FGameplayAbilitySpec* FoundSpec = nullptr;
	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const bool bMatch = Spec.Ability && (Spec.Ability->GetAssetTags().HasTag(TargetAbilityTag) || Spec.Ability->AbilityTags.HasTag(TargetAbilityTag));
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		if (bMatch)
		{
			FoundSpec = &Spec;
			break;
		}
	}

	if (!FoundSpec) return;

	// Consume cost
	if (NextUpgrade.CostType == EGSAUpgradeCostType::Money)
	{
		AGSGameState* GSGameState = Cast<AGSGameState>(GetWorld()->GetGameState());
		if (GSGameState)
		{
			GSGameState->AddMoneyDirectly(-static_cast<float>(NextUpgrade.MoneyCost));
		}
	}
	else if (NextUpgrade.CostType == EGSAUpgradeCostType::Object)
	{
		if (!ConsumeOverlappingObjects(NextUpgrade.RequiredObjectTag, NextUpgrade.RequiredQuantity))
		{
			UE_LOG(LogTemp, Error, TEXT("UpgradeStation: Failed to consume objects even after CanUpgrade checked out."));
			OnUpgradeFailed.Broadcast(PlayerPawn, FText::FromString(TEXT("Error al consumir los objetos.")));
			return;
		}
	}

	// Apply upgrade level
	FoundSpec->Level = NextUpgrade.TargetLevel;
	ASC->MarkAbilitySpecDirty(*FoundSpec);

	UE_LOG(LogTemp, Log, TEXT("UpgradeStation: Successfully upgraded ability %s to level %d for %s"), 
		*TargetAbilityTag.ToString(), NextUpgrade.TargetLevel, *PlayerPawn->GetName());

	OnUpgradeSuccessful.Broadcast(PlayerPawn, NextUpgrade.TargetLevel);

	// Immediately check if the new level reached max level to send the notification
	if (NextUpgrade.TargetLevel >= UpgradeData->GetMaxLevel())
	{
		UE_LOG(LogTemp, Log, TEXT("UpgradeStation: Player reached max level (%d) for ability %s after upgrade"), NextUpgrade.TargetLevel, *TargetAbilityTag.ToString());
		OnMaxLevelReached.Broadcast(PlayerPawn);
	}
}

bool AGSAbilityUpgradeStation::ActorMatchesTag(AActor* Actor, const FGameplayTag& RequiredTag) const
{
	if (!Actor || !RequiredTag.IsValid()) return false;

	// Check 1: ActorHasTag
	if (Actor->ActorHasTag(RequiredTag.GetTagName()))
	{
		return true;
	}

	// Check 2: IGameplayTagAssetInterface
	if (IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Actor))
	{
		if (TagInterface->HasMatchingGameplayTag(RequiredTag))
		{
			return true;
		}
	}

	// Check 3: AGSItem ItemTags
	if (AGSItem* Item = Cast<AGSItem>(Actor))
	{
		if (Item->ItemTags.HasTag(RequiredTag))
		{
			return true;
		}
	}

	// Check 4: Actor ASC tags
	if (IAbilitySystemInterface* ActorASCInterface = Cast<IAbilitySystemInterface>(Actor))
	{
		if (UAbilitySystemComponent* ActorASC = ActorASCInterface->GetAbilitySystemComponent())
		{
			if (ActorASC->HasMatchingGameplayTag(RequiredTag))
			{
				return true;
			}
		}
	}

	return false;
}

bool AGSAbilityUpgradeStation::ConsumeOverlappingObjects(const FGameplayTag& RequiredTag, int32 Quantity)
{
	TArray<AActor*> OverlappingActors;
	TriggerVolume->GetOverlappingActors(OverlappingActors);

	int32 ConsumedCount = 0;
	for (AActor* Actor : OverlappingActors)
	{
		if (Actor && ActorMatchesTag(Actor, RequiredTag))
		{
			Actor->Destroy();
			ConsumedCount++;
			if (ConsumedCount >= Quantity)
			{
				return true;
			}
		}
	}

	return ConsumedCount >= Quantity;
}
