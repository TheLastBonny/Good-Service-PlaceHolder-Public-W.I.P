#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Blueprint/UserWidget.h"
#include "GSAbility_EmoteMenu.generated.h"

class UGSEmoteLibrary;
class UGSEmoteDefinition;
class UGSAbility_EmoteMenu;

UCLASS(Blueprintable, BlueprintType)
class PROJECTF_API UGSEmoteMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Emote Menu", meta = (ExposeOnSpawn = true))
	TObjectPtr<UGSAbility_EmoteMenu> OwningEmoteAbility;

	UFUNCTION(BlueprintImplementableEvent, Category = "Emote Menu")
	void OnInitMenu(UGSEmoteLibrary* Library);
};

UCLASS()
class PROJECTF_API UGSAbility_EmoteMenu : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGSAbility_EmoteMenu();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable, Category = "Emote Menu")
	void SelectEmote(UGSEmoteDefinition* EmoteDef);

	UFUNCTION(BlueprintCallable, Category = "Emote Menu")
	void CancelMenu();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote Menu")
	TSubclassOf<UGSEmoteMenuWidget> EmoteMenuWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote Menu")
	TObjectPtr<UGSEmoteLibrary> EmoteLibrary;

	UPROPERTY()
	TObjectPtr<UGSEmoteMenuWidget> CreatedWidget;

private:
	void SetInputMode(bool bShowMouseCursor);
};
