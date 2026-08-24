#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "DataAssets/GSAbilityUpgradeDataAsset.h"
#include "GSAbilityUpgradeStation.generated.h"

class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUpgradeSuccessfulSignature, APawn*, Player, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUpgradeFailedSignature, APawn*, Player, const FText&, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxLevelReachedSignature, APawn*, Player);

UCLASS()
class PROJECTF_API AGSAbilityUpgradeStation : public AActor
{
	GENERATED_BODY()

public:
	AGSAbilityUpgradeStation();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	/** The tag of the ability this machine upgrades (e.g. Ability.Grab, Ability.Launch). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Config")
	FGameplayTag TargetAbilityTag;

	/** Data asset containing the progression cost configurations. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Config")
	TObjectPtr<UGSAbilityUpgradeDataAsset> UpgradeData;

public:
	/** Broadcasts when the player successfully upgrades the ability. */
	UPROPERTY(BlueprintAssignable, Category = "Upgrade Events")
	FOnUpgradeSuccessfulSignature OnUpgradeSuccessful;

	/** Broadcasts when an upgrade attempt fails. */
	UPROPERTY(BlueprintAssignable, Category = "Upgrade Events")
	FOnUpgradeFailedSignature OnUpgradeFailed;

	/** Broadcasts when the player is already at the maximum level of this ability. */
	UPROPERTY(BlueprintAssignable, Category = "Upgrade Events")
	FOnMaxLevelReachedSignature OnMaxLevelReached;

	/** Attempts to upgrade the target ability of the given player pawn. Server-only logic. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Upgrade")
	void AttemptUpgrade(APawn* PlayerPawn);

	/** Helper to check if a pawn can upgrade, returns cost details. */
	UFUNCTION(BlueprintPure, Category = "Upgrade")
	bool CanUpgrade(APawn* PlayerPawn, FText& OutFailReason, FGSAbilityUpgradeLevel& OutNextUpgradeInfo) const;

protected:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	/** Helper to determine if an actor matches the specified tag. */
	bool ActorMatchesTag(AActor* Actor, const FGameplayTag& RequiredTag) const;

	/** Helper to find and consume a specified quantity of matching actors inside the trigger volume. */
	bool ConsumeOverlappingObjects(const FGameplayTag& RequiredTag, int32 Quantity);
};
