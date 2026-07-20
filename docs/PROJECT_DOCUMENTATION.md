# Project Documentation: Movement Architecture (Mover), GAS, and StateTree AI (Prepared for Unreal Engine 6.0)

This documentation comprehensively analyzes the core architecture of the project systems. It is structured as an in-depth technical guide to understand the implementation of the **Mover** movement component, its bidirectional integration with the **Gameplay Ability System (GAS)**, and behavior automation using **StateTree AI**.
Furthermore, it explores how these systems are designed for a seamless transition toward **Verse in Unreal Engine 6.0**.

---

## 1. Introduction and Mover Architecture

In traditional Unreal Engine development, character movement has been managed via `CharacterMovementComponent`. Although powerful, this component suffers from extensibility and architectural rigidity due to a monolithic inheritance hierarchy built over a decade ago. Extending it for non-humanoid physics, custom kinematic behaviors, or vehicle systems requires modifying thousands of lines of base code or resorting to complex workarounds.

To overcome these limitations, Unreal Engine introduces the **Mover** framework (via `UCharacterMoverComponent`). This modern architecture is characterized by being:
* **Modular and Extensible:** Movement is broken down into independent "Movement Modes" (`Movement Modes`) that can be added or removed at runtime.
* **Constraint-Based:** The physics solver calculates forces and collisions by applying explicit kinematic constraints.
* **Natively Integrated with Network Prediction:** Mover connects directly to Epic Games' **Network Prediction** plugin, providing local client prediction, authoritative server replication, and automatic network reconciliation (rollback and replay) without requiring manual network programming by the developer.

### The Integration Challenge: Mover vs. Chaos Physics (Mover Chaos)

During the development of this project, one of the major technical challenges was the coexistence between the Mover component and Unreal Engine's physics engine, **Chaos Physics**.

Chaos Physics simulates dynamic rigid bodies asynchronously using independent physics sub-threads (**Physics Substepping**), which greatly improves the stability of complex collisions and destruction in Unreal. However, this asynchrony directly conflicts with Mover's network prediction and reconciliation system:
1. **Lack of Determinism in Re-simulations (Rollback):** When a client experiences network desynchronization and Mover triggers a rollback to re-simulate recent frames, Chaos Physics cannot rewind the state of the rigid body deterministically and synchronously at the same time. This causes recurrent prediction errors, visual jittering, and collision misalignments.
2. **Kinematic vs. Dynamic:** Due to these limitations with dynamic Chaos in predictive networking, we opted to force interactive objects under transport to behave kinematically (`LaunchKinematic`), calculating explicit step-by-step parabolic trajectories rather than delegating free physics flight to Chaos.
3. **Future Outlook:** Epic Games is working on unifying the physics pipeline where Mover will interact directly with a network-predictable Chaos solver. This will enable real-time dynamic physical body simulation with rollback without requiring manual kinematic interpolations.

> [!NOTE]
> **Verse Perspective in Unreal Engine 6.0: Modular Movement Programming**
> With the native arrival of the **Verse language in Unreal Engine 6.0**, Mover's architecture will evolve dramatically. Movement Modes will no longer rely on complex C++ inheritance or rigid Blueprints. Verse will structure movement through highly-typed, concurrent scripts attached dynamically. Its static verification system will ensure physics safety at compile time, eliminating pointer errors during Mover transitions.

### Comparison Table: Classic Movement vs. Mover Component

| Feature | CharacterMovementComponent (Classic) | Mover Component (Modern) |
| :--- | :--- | :--- |
| **Architecture** | Monolithic, difficult to modify or extend. | Modular, component-based, uses Movement Modes. |
| **Network Prediction** | Rigidly coupled to the `ACharacter` class. | Decoupled, delegated to **Network Prediction** plugin for any pawn. |
| **Movement Modes** | Limited to predefined enums (`MOVE_Walking`, `MOVE_Falling`, etc.). | Independent and unlimited C++ classes inheriting from `UBaseMovementMode`. |
| **Physics** | Direct translations, implicitly integrated. | Based on precise physics constraint solvers. |
| **Blackboard Memory** | Loose class variables difficult to replicate across network. | Dynamic, replicable blackboard (`UMoverBlackboard`). |

### Pawn Movement Components (`AGSPawn`)

In the project, the primary pawn `AGSPawn` utilizes the following component declarations in its header (`GSPawn.h`):
```cpp
protected:
	/** Component managing capsule collision in the world */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	/** Core Mover component resolving pawn physics */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCharacterMoverComponent> MoverComponent;

	/** Navigation component translating AI pathing intent to Mover */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNavMoverComponent> NavMoverComponent;
```

The constructor in `AGSPawn.cpp` initializes them and sets initial collision and replication configurations:
```cpp
AGSPawn::AGSPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Collision capsule initialization
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	CapsuleComponent->InitCapsuleSize(35.f, 90.f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	CapsuleComponent->SetShouldUpdatePhysicsVolume(true);
	CapsuleComponent->SetCanEverAffectNavigation(true);
	CapsuleComponent->bDynamicObstacle = true;
	RootComponent = CapsuleComponent;

	// Movement component initialization for the new architecture
	MoverComponent = CreateDefaultSubobject<UCharacterMoverComponent>(TEXT("MoverComponent"));
	NavMoverComponent = CreateDefaultSubobject<UNavMoverComponent>(TEXT("NavMoverComponent"));

	// Pawn is network replicated, but physical movement is delegated to Network Prediction
	SetReplicateMovement(false);
	bReplicates = true;
}
```

---

## 2. Networked Kinematic Input System (Input Cmd Context)

The Mover simulation advances frame-by-frame based on an immutable per-frame input structure known as `FMoverInputCmdContext`. This input command encapsulates all logical and analog intentions of the pawn for that specific simulation instant.

### IMoverInputProducerInterface and Predictive Input

The game client does not directly apply forces or translations to the pawn in response to key presses. Instead, the pawn implements the `IMoverInputProducerInterface` interface and the `ProduceInput_Implementation` method.

This method packages physical keyboard/gamepad presses or AI navigation directives inside a generic container named `FMoverDataCollection` within the output variable `InputCmdResult`.

```
[Keyboard / AI Input] 
        |
        v
[ProduceInput_Implementation]
        |
        v  (Packages into FCharacterDefaultInputs)
[FMoverInputCmdContext]  --------> [Local Network Buffer (Prediction)]
        |
        v  (Simulates locally)
[Mover Simulation Tick] <=== (Re-executes during Rollback/Replay)
```

> [!NOTE]
> **Verse Perspective in Unreal Engine 6.0: Reactive Input Processing**
> In **Unreal Engine 6.0**, Mover's classic polling cycle will be replaced by asynchronous event streams. Instead of populating `FMoverInputCmdContext` by copying variables frame-by-frame, Verse will allow defining streams that listen directly to actions (`OnActionClicked`) and propagate instant signals to Mover via concurrent channels, optimizing CPU usage.

### Input Flow Analysis in `AGSPawn`

The `ProduceInput_Implementation` method executes cyclically to feed Mover's physics solver:

```cpp
void AGSPawn::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	/* 
	 * 1. Emote Cancellation via Movement (Mover + GAS Synergy)
	 * We check if GAS reports that the user is in the "Emoting" state.
	 * If Mover detects new movement input or jump, we force
	 * cancellation of GAS abilities associated with Emotes.
	 */
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(GSGameplayTags::State_Emoting))
	{
		if (!CachedMovementInput.IsZero() || bCachedJumpPressed)
		{
			FGameplayTagContainer EmotingTags;
			EmotingTags.AddTag(GSGameplayTags::State_Emoting);
			AbilitySystemComponent->CancelAbilities(&EmotingTags);
		}
	}

	// Obtain standard Mover character input structure
	FCharacterDefaultInputs& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();
	FVector MoveDirection = FVector::ZeroVector;

	// 2. Artificial Intelligence Pathing (NPCs)
	// If the pawn is AI-controlled, NavMoverComponent extracts pathfinding movement intent
	// and assigns it to MoveDirection.
	if (NavMoverComponent)
	{
		FVector NavMoveInputIntent = FVector::ZeroVector;
		FVector NavMoveInputVelocity = FVector::ZeroVector;
		if (NavMoverComponent->ConsumeNavMovementData(NavMoveInputIntent, NavMoveInputVelocity))
		{
			MoveDirection = NavMoveInputIntent;
		}
	}

	// 3. Human Player Pathing
	// If there is no AI navigation and player provides movement input,
	// we project 2D input into 3D space based on camera orientation.
	if (MoveDirection.IsNearlyZero() && !CachedMovementInput.IsZero())
	{
		FRotator BaseRotation = FRotator::ZeroRotator;
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			BaseRotation = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation() : PC->GetControlRotation();
		}
		else
		{
			BaseRotation = GetControlRotation();
		}

		// Project movement onto horizontal plane (ignoring Pitch/Roll)
		const FRotator YawRotation(0.f, BaseRotation.Yaw, 0.f);
		const FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		MoveDirection = (ForwardVector * CachedMovementInput.Y) + (RightVector * CachedMovementInput.X);
		MoveDirection.Normalize();
	}

	// Set physical locomotion direction and jump states
	CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, MoveDirection);
	CharacterInputs.bIsJumpPressed = bCachedJumpPressed;
	CharacterInputs.bIsJumpJustPressed = bCachedJumpJustPressed;

	// 4. Physical Orientation: GAS Aiming vs. Locomotion
	// If player is aiming to launch an object (holds State_Aiming tag),
	// we calculate vector toward mouse cursor and direct Mover to orient pawn accordingly.
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(GSGameplayTags::State_Aiming))
	{
		FVector AimDirection = GetActorForwardVector();
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FHitResult HitResult;
			if (PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
			{
				FVector Dir = HitResult.Location - GetActorLocation();
				Dir.Z = 0.0f;
				if (Dir.Normalize())
				{
					AimDirection = Dir;
				}
			}
		}
		CharacterInputs.OrientationIntent = AimDirection;
	}
	else
	{
		// Standard orientation: pawn faces movement direction
		CharacterInputs.OrientationIntent = MoveDirection;
	}

	CharacterInputs.ControlRotation = GetControlRotation();
	bCachedJumpJustPressed = false;
}
```

---

## 3. Mover Simulation States and Network Reconciliation

The core of Mover simulation evolves physical state through low-level transition structures.

### `FMoverSyncState` Structure
Defines volatile data replicated and verified over network each frame:
* `MovementMode`: Active simulation mode name (`FName`, e.g., `Walking` or `Falling`).
* `LayeredMoves`: Cumulative layered movements (impulses, kinematic launches, rope pulls).
* `MovementModifiers`: External modifiers applied to Mover's solver.

### `FMovementModeTickEndState` Structure
At the end of a movement mode simulation step, it returns this structure with key information:
1. `NextModeName`: Target mode name if a transition was triggered.
2. `RemainingMs`: Remaining milliseconds in the frame's simulation tick. If a character walks off a ledge halfway through a frame tick, walk mode ends, returns remaining milliseconds in `RemainingMs`, and Mover activates falling mode (`Falling`) to consume remaining time in the exact same physics frame.

```cpp
USTRUCT(BlueprintType)
struct FMovementModeTickEndState
{
	GENERATED_BODY()

	// Unused milliseconds in current mode execution
	UPROPERTY(BlueprintReadWrite, Category=Mover)
	float RemainingMs;

	// Next mode to transition to within the same Tick
	UPROPERTY(BlueprintReadWrite, Category=Mover)
	FName NextModeName = NAME_None;

	// Optimization flag: indicates if physical state was unchanged
	UPROPERTY(BlueprintReadWrite, Category=Mover)
	bool bEndedWithNoChanges = false;
};
```

### Client Prediction, Rollback, and Reconciliation
Mover executes local movement simulation on the client predictively. Subsequently, client sends input commands (`FMoverInputCmdContext`) to the server over UDP.
The server processes input authoritatively and responds by replicating the actual server synchronization state (`FMoverSyncState`). The client intercepts this state via:

```cpp
bool FMoverSyncState::ShouldReconcile(const FMoverSyncState& AuthorityState) const
{
	return (MovementMode != AuthorityState.MovementMode) || 
	       SyncStateCollection.ShouldReconcile(AuthorityState.SyncStateCollection) ||
	       MovementModifiers.ShouldReconcile(AuthorityState.MovementModifiers);
}
```

If a mismatch occurs (due to latency, server-side collision, or unexpected mode change):
1. **Rollback:** Client discards erroneous local physical position and snaps position and mode to server's `AuthorityState`.
2. **Replay:** Client rapidly re-executes all local input commands sent to server but not yet acknowledged, correcting trajectory invisibly to the player.

> [!NOTE]
> **Verse Perspective in Unreal Engine 6.0: Reconciliation via STM**
> Unreal Engine 6.0 will introduce native Verse support based on **Software Transactional Memory (STM)**. Rather than writing custom `ShouldReconcile` logic or handling manual rollbacks in C++, prediction will be treated as atomic memory transactions. If network desync occurs, simulation rolls back state automatically.

### Dynamic Movement Bases (Blackboard)
When a character interacts with moving bases (e.g., moving platforms or held objects), Mover records this relationship in its local database (`UMoverBlackboard`). To prevent physics drag artifacts, this relationship is explicitly cleared when executing abilities like Grab:
```cpp
if (UCharacterMoverComponent* MoverComp = AvatarActor->FindComponentByClass<UCharacterMoverComponent>())
{
	if (UMoverBlackboard* SimBlackboard = MoverComp->GetSimBlackboard_Mutable())
	{
		FRelativeBaseInfo EmptyBaseInfo;
		SimBlackboard->Set(CommonBlackboard::LastFoundDynamicMovementBase, EmptyBaseInfo);
	}
}
```

---

## 4. Mover + GAS Synergy: Bidirectional Integration

The core strength of the project lies in the convergence of **Mover and GAS**. Communication between Ability System and movement solver is continuous and real-time.

### Reactive Binding of `WalkSpeed` Attribute
`AGSPawn` initializes GAS data in `InitAbilityActorInfo`. During this phase, rather than polling attribute values in `Tick`, we subscribe to GAS numeric change delegates to react only when attributes change.

```cpp
void AGSPawn::InitAbilityActorInfo()
{
	if (!AbilitySystemComponent) { return; }

	if (APlayerState* PS = GetPlayerState())
	{
		AbilitySystemComponent->InitAbilityActorInfo(PS, this);
	}
	else
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	if (MovementSet)
	{
		// Clean up previous subscriptions
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			MovementSet->GetWalkSpeedAttribute()).RemoveAll(this);

		// Subscribe to WalkSpeed change delegate
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			MovementSet->GetWalkSpeedAttribute()).AddUObject(this, &AGSPawn::OnWalkSpeedChanged);

		// Synchronize initial Mover max speed
		if (MoverComponent)
		{
			if (UCommonLegacyMovementSettings* MoveSettings = MoverComponent->FindSharedSettings_Mutable<UCommonLegacyMovementSettings>())
			{
				MoveSettings->MaxSpeed = MovementSet->GetWalkSpeed();
			}
		}
	}
}
```

### Dynamic Speed Application
When a Gameplay Effect alters the speed attribute (e.g., `GE_CaffeineBoost` or `GE_Encumbered`), GAS automatically fires listener function:
```cpp
void AGSPawn::OnWalkSpeedChanged(const FOnAttributeChangeData& Data)
{
	if (MoverComponent)
	{
		if (UCommonLegacyMovementSettings* MoveSettings = MoverComponent->FindSharedSettings_Mutable<UCommonLegacyMovementSettings>())
		{
			MoveSettings->MaxSpeed = Data.NewValue;
		}
	}
}
```

---

## 5. Artificial Intelligence Control via StateTree

NPC customer workflow is governed hierarchically and efficiently via **StateTree**. StateTree simplifies architecture compared to traditional Behavior Trees by providing flat decision trees and compact instance data memory footprint.

NPC behavior is defined by C++ tasks inheriting from `FStateTreeTaskCommonBase`:

```
[NPC Spawns] 
     |
     v
[State: Entrance]  ===> FGSStateTreeTask_AssignTable (Reserves table spot)
     |
     v
[State: Order]     ===> FGSStateTreeTask_ChooseRandomOrder (Picks recipe from menu)
     |
     v
[State: Wait]      ===> NPCComponent starts patience timer
     |
     v  (Receives Food / Patience Expires)
[State: Exit]      ===> FGSStateTreeTask_SendToExit (Frees table & walks to exit)
```

### Table Assignment Task (`GSStateTreeTask_AssignTable`)
Requests an open table from central manager `AGSNPCManager`. If assigned, updates navigation blackboard and returns success to transition to next state.

```cpp
EStateTreeRunStatus FGSStateTreeTask_AssignTable::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.bSuccess = false;

	if (AActor* Actor = InstanceData.Actor)
	{
		if (APawn* Pawn = Cast<APawn>(Actor))
		{
			if (UWorld* World = Actor->GetWorld())
			{
				if (AGSGameState* GameState = Cast<AGSGameState>(World->GetGameState()))
				{
					if (AGSNPCManager* NPCManager = GameState->GetNPCManager())
					{
						bool bAssigned = NPCManager->AssignTableToNPC(Pawn);
						InstanceData.bSuccess = bAssigned;
						return bAssigned ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
					}
				}
			}
		}
	}
	return EStateTreeRunStatus::Failed;
}
```

### Order Selection Task (`GSStateTreeTask_ChooseRandomOrder`)
Instructs NPC component (`UGSNPCComponent`) to pick a meal and trigger order UI overhead.
```cpp
EStateTreeRunStatus FGSStateTreeTask_ChooseRandomOrder::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (AActor* Actor = InstanceData.Actor)
	{
		if (UGSNPCComponent* NPCComp = Actor->FindComponentByClass<UGSNPCComponent>())
		{
			NPCComp->ChooseRandomOrder();
			return EStateTreeRunStatus::Succeeded;
		}
	}
	return EStateTreeRunStatus::Failed;
}
```

### Send To Exit Task (`GSStateTreeTask_SendToExit`)
Releases table reservation in manager and directs NPC to walk toward dispatch area.
```cpp
EStateTreeRunStatus FGSStateTreeTask_SendToExit::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (AActor* Actor = InstanceData.Actor)
	{
		if (APawn* Pawn = Cast<APawn>(Actor))
		{
			if (UWorld* World = Actor->GetWorld())
			{
				if (AGSGameState* GameState = Cast<AGSGameState>(World->GetGameState()))
				{
					if (AGSNPCManager* NPCManager = GameState->GetNPCManager())
					{
						NPCManager->SendNPCToExit(Pawn);
						return EStateTreeRunStatus::Succeeded;
					}
				}
			}
		}
	}
	return EStateTreeRunStatus::Failed;
}
```
