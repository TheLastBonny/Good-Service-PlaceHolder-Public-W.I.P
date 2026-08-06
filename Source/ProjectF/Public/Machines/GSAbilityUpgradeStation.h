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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Config")
	FGameplayTag TargetAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Config")
	TObjectPtr<UGSAbilityUpgradeDataAsset> UpgradeData;

public:
	UPROPERTY(BlueprintAssignable, Category = "Upgrade Events")
	FOnUpgradeSuccessfulSignature OnUpgradeSuccessful;

	UPROPERTY(BlueprintAssignable, Category = "Upgrade Events")
	FOnUpgradeFailedSignature OnUpgradeFailed;

	UPROPERTY(BlueprintAssignable, Category = "Upgrade Events")
	FOnMaxLevelReachedSignature OnMaxLevelReached;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Upgrade")
	void AttemptUpgrade(APawn* PlayerPawn);

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	bool CanUpgrade(APawn* PlayerPawn, FText& OutFailReason, FGSAbilityUpgradeLevel& OutNextUpgradeInfo) const;

protected:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	bool ActorMatchesTag(AActor* Actor, const FGameplayTag& RequiredTag) const;

	bool ConsumeOverlappingObjects(const FGameplayTag& RequiredTag, int32 Quantity);
};
