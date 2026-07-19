#include "Abilities/GSAbility_EmoteMenu.h"
#include "AbilitySystemComponent.h"
#include "DataAssets/GSEmoteDefinition.h"
#include "Core/GSGameplayTags.h"
#include "GameFramework/PlayerController.h"

UGSAbility_EmoteMenu::UGSAbility_EmoteMenu()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

void UGSAbility_EmoteMenu::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!ActorInfo || !ActorInfo->IsLocallyControlled())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	APlayerController* PC = ActorInfo->PlayerController.Get();
	if (!PC || !EmoteMenuWidgetClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CreatedWidget = CreateWidget<UGSEmoteMenuWidget>(PC, EmoteMenuWidgetClass);
	if (CreatedWidget)
	{
		CreatedWidget->OwningEmoteAbility = this;
		CreatedWidget->AddToViewport();
		CreatedWidget->OnInitMenu(EmoteLibrary);
		SetInputMode(true);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UGSAbility_EmoteMenu::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (CreatedWidget)
	{
		CreatedWidget->RemoveFromParent();
		CreatedWidget = nullptr;
	}

	SetInputMode(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGSAbility_EmoteMenu::SelectEmote(UGSEmoteDefinition* EmoteDef)
{
	if (EmoteDef && CurrentActorInfo)
	{
		UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
		if (ASC)
		{
			FGameplayEventData Payload;
			Payload.Instigator = GetAvatarActorFromActorInfo();
			Payload.Target = GetAvatarActorFromActorInfo();
			Payload.OptionalObject = EmoteDef;

			ASC->HandleGameplayEvent(GSGameplayTags::Event_Ability_PlayEmote, &Payload);
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGSAbility_EmoteMenu::CancelMenu()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGSAbility_EmoteMenu::SetInputMode(bool bShowMouseCursor)
{
	APlayerController* PC = CurrentActorInfo ? CurrentActorInfo->PlayerController.Get() : nullptr;
	if (PC)
	{
		if (bShowMouseCursor)
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
		else
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;
		}
	}
}
