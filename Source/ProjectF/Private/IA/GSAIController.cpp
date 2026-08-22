#include "IA/GSAIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "DefaultMovementSet/NavMoverComponent.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"

AGSAIController::AGSAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	bReplicates = true;

	if (UCrowdFollowingComponent* CrowdComp = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
	{
		CrowdComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Good);
		CrowdComp->SetCrowdCollisionQueryRange(400.0f);
		CrowdComp->SetCrowdSeparation(true);
		CrowdComp->SetCrowdSeparationWeight(2.0f);
		CrowdComp->SetCrowdObstacleAvoidance(true);
		CrowdComp->SetCrowdOptimizeTopology(true);
		CrowdComp->SetCrowdOptimizeVisibility(true);
		CrowdComp->SetCrowdAnticipateTurns(true);
	}
}

const FNavAgentProperties& AGSAIController::GetNavAgentPropertiesRef() const
{
	if (APawn* MyPawn = GetPawn())
	{
		if (UCapsuleComponent* Capsule = MyPawn->FindComponentByClass<UCapsuleComponent>())
		{
			CachedNavAgentProps.AgentRadius = FMath::Min(Capsule->GetUnscaledCapsuleRadius(), 35.0f);
			CachedNavAgentProps.AgentHeight = FMath::Min(Capsule->GetUnscaledCapsuleHalfHeight() * 2.0f, 180.0f);
			return CachedNavAgentProps;
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			if (ANavigationData* NavData = NavSys->GetDefaultNavDataInstance())
			{
				return NavData->GetConfig();
			}
		}
	}

	CachedNavAgentProps.AgentRadius = 35.0f;
	CachedNavAgentProps.AgentHeight = 180.0f;
	return CachedNavAgentProps;
}

void AGSAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (InPawn)
	{
		if (UNavMoverComponent* NavMover = InPawn->FindComponentByClass<UNavMoverComponent>())
		{
			NavMover->SetPathFollowingAgent(GetPathFollowingComponent());
			if (UCapsuleComponent* Capsule = InPawn->FindComponentByClass<UCapsuleComponent>())
			{
				NavMover->NavAgentProps.AgentRadius = FMath::Min(Capsule->GetUnscaledCapsuleRadius(), 35.0f);
				NavMover->NavAgentProps.AgentHeight = FMath::Min(Capsule->GetUnscaledCapsuleHalfHeight() * 2.0f, 180.0f);
			}
		}
	}

	UCrowdFollowingComponent* CrowdComp = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent());
	const FNavAgentProperties& AgentProps = GetNavAgentPropertiesRef();
	UE_LOG(LogTemp, Warning, TEXT("[GSAIController] Possessed Pawn '%s' | PathFollowing: %s | AgentRadius: %.1f Height: %.1f"),
		InPawn ? *InPawn->GetName() : TEXT("NULL"),
		CrowdComp ? TEXT("Detour CrowdAvoidance ACTIVE") : TEXT("Standard PathFollowing"),
		AgentProps.AgentRadius,
		AgentProps.AgentHeight);
}
