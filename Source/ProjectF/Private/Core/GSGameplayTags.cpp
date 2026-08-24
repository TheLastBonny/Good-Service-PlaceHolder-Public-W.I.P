#include "Core/GSGameplayTags.h"

namespace GSGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Input_Action_Skill1, "Input.Action.Skill1");
	UE_DEFINE_GAMEPLAY_TAG(Input_Action_Skill2, "Input.Action.Skill2");
	UE_DEFINE_GAMEPLAY_TAG(Ability_KineticRope, "Ability.KineticRope");
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");
	UE_DEFINE_GAMEPLAY_TAG(State_Raw, "State.Condition.Cook.Raw");
	UE_DEFINE_GAMEPLAY_TAG(State_Cooked, "State.Condition.Cook.Cooked");
	UE_DEFINE_GAMEPLAY_TAG(State_Burned, "State.Condition.Cook.Burned");
	UE_DEFINE_GAMEPLAY_TAG(State_Cooled, "State.Condition.Cook.Cooled");
	UE_DEFINE_GAMEPLAY_TAG(State_Filled, "State.Condition.Liquid.Filled");
	UE_DEFINE_GAMEPLAY_TAG(State_Aiming, "State.Aiming");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Grab, "Ability.Grab");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Launch, "Ability.Launch");
	UE_DEFINE_GAMEPLAY_TAG(State_HoldingItem, "State.HoldingItem");
	UE_DEFINE_GAMEPLAY_TAG(State_Catching, "State.Catching");
	UE_DEFINE_GAMEPLAY_TAG(Data_MoneyAmount, "Data.MoneyAmount");
	UE_DEFINE_GAMEPLAY_TAG(Input_Action_EmoteMenu, "Input.Action.EmoteMenu");
	UE_DEFINE_GAMEPLAY_TAG(Event_Ability_PlayEmote, "Event.Ability.PlayEmote");
	UE_DEFINE_GAMEPLAY_TAG(State_Emoting, "State.Emoting");

	// Game Phase Tags
	UE_DEFINE_GAMEPLAY_TAG(GamePhase_Core_WaitingToStart, "GamePhase.Core.WaitingToStart");
	UE_DEFINE_GAMEPLAY_TAG(GamePhase_Core_RoundInProgress, "GamePhase.Core.RoundInProgress");
	UE_DEFINE_GAMEPLAY_TAG(GamePhase_Core_RoundOver, "GamePhase.Core.RoundOver");
	UE_DEFINE_GAMEPLAY_TAG(GamePhase_Event, "GamePhase.Event");
}

