#include "Abilities/GSAbility_PlayEmote.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "DataAssets/GSEmoteDefinition.h"
#include "Characters/GSPawn.h"
#include "Core/GSGameplayTags.h"
#include "TimerManager.h"

UGSAbility_PlayEmote::UGSAbility_PlayEmote()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Add State.Emoting as an activation owned tag.
	// This means while this ability is active, the owner will automatically have this tag.
	ActivationOwnedTags.AddTag(GSGameplayTags::State_Emoting);

	// Triggering setup: trigger when receiving Event.Ability.PlayEmote
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = GSGameplayTags::Event_Ability_PlayEmote;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGSAbility_PlayEmote::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!TriggerEventData || !TriggerEventData->OptionalObject)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const UGSEmoteDefinition* EmoteDef = Cast<UGSEmoteDefinition>(TriggerEventData->OptionalObject);
	if (!EmoteDef)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Play music/sound replicated via Pawn multicast RPC
	AGSPawn* Pawn = Cast<AGSPawn>(ActorInfo->AvatarActor.Get());
	if (Pawn && HasAuthority(&ActivationInfo))
	{
		Pawn->MulticastPlayEmoteSound(const_cast<UGSEmoteDefinition*>(EmoteDef));
	}

	// Play animation montage
	UAnimMontage* MontageToPlay = EmoteDef->EmoteMontage.LoadSynchronous();
	const bool bIsLocallyControlled = ActorInfo && ActorInfo->IsLocallyControlled();

	if (MontageToPlay && bIsLocallyControlled)
	{
		UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			TEXT("PlayEmoteMontage"),
			MontageToPlay,
			1.f,
			NAME_None,
			true,
			1.f
		);

		if (PlayMontageTask)
		{
			PlayMontageTask->OnCompleted.AddDynamic(this, &UGSAbility_PlayEmote::OnMontageFinished);
			PlayMontageTask->OnBlendOut.AddDynamic(this, &UGSAbility_PlayEmote::OnMontageFinished);
			PlayMontageTask->OnInterrupted.AddDynamic(this, &UGSAbility_PlayEmote::OnMontageInterrupted);
			PlayMontageTask->OnCancelled.AddDynamic(this, &UGSAbility_PlayEmote::OnMontageInterrupted);
			PlayMontageTask->ReadyForActivation();
		}
		else
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		}
	}
	else
	{
		// Fallback if no montage (or server instance): end after the montage or sound duration
		float Duration = 3.f;
		if (MontageToPlay)
		{
			Duration = MontageToPlay->GetPlayLength();
		}
		else if (USoundBase* Sound = EmoteDef->EmoteSound.LoadSynchronous())
		{
			Duration = Sound->GetDuration();
		}
		
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(EmoteTimerHandle, this, &UGSAbility_PlayEmote::OnEmoteTimerFinished, Duration, false);
		}
		else
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		}
	}
}

void UGSAbility_PlayEmote::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EmoteTimerHandle);
	}

	// Stop music/sound replicated via Pawn multicast RPC
	if (ActorInfo)
	{
		AGSPawn* Pawn = Cast<AGSPawn>(ActorInfo->AvatarActor.Get());
		if (Pawn && HasAuthority(&ActivationInfo))
		{
			Pawn->MulticastStopEmoteSound();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGSAbility_PlayEmote::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGSAbility_PlayEmote::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGSAbility_PlayEmote::OnEmoteTimerFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
