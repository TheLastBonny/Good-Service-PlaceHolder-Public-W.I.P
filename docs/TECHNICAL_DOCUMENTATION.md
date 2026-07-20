# Mover Component & Gameplay Ability System (GAS) Technical Framework Guide

Welcome to the technical reference manual for the modular movement, physical simulation, and Gameplay Ability System (GAS) framework in Unreal Engine 5.8.

This document is structured as an architectural deep-dive—modeled after community standards such as Tranek's GAS Documentation—providing modular explanations of engine systems, networked physics prediction, bidirectional GAS bindings, and upcoming migration paths toward Verse in Unreal Engine 6.0.

---

## Table of Contents

1. [Mover Framework Architecture](#1-mover-framework-architecture)
   - [1.1 Overview & Legacy CharacterMovement Comparison](#11-overview--legacy-charactermovement-comparison)
   - [1.2 Movement Modes & Blackboard Memory](#12-movement-modes--blackboard-memory)
   - [1.3 Input Command Context & Network Prediction](#13-input-command-context--network-prediction)
   - [1.4 Reconciliation, Rollback, and Replay](#14-reconciliation-rollback-and-replay)
   - [1.5 Mover vs. Chaos Physics Coexistence](#15-mover-vs-chaos-physics-coexistence)
2. [Bidirectional Mover + GAS Integration](#2-bidirectional-mover--gas-integration)
   - [2.1 Reactive Attribute Binding (WalkSpeed)](#21-reactive-attribute-binding-walkspeed)
   - [2.2 Locomotion Input & Ability Cancellation](#22-locomotion-input--ability-cancellation)
   - [2.3 Kinematic Trajectory Simulation & Stacking Equations](#23-kinematic-trajectory-simulation--stacking-equations)
3. [Verse & Unreal Engine 6.0 Paradigm Shift](#3-verse--unreal-engine-60-paradigm-shift)
   - [3.1 Reactive Input Streams vs. Polling Cycles](#31-reactive-input-streams-vs-polling-cycles)
   - [3.2 Software Transactional Memory (STM) Reconciliation](#32-software-transactional-memory-stm-reconciliation)
   - [3.3 Concurrent Async Coroutines for Physics](#33-concurrent-async-coroutines-for-physics)
4. [Extending the Framework](#4-extending-the-framework)
   - [4.1 Creating Custom Movement Modes in C++](#41-creating-custom-movement-modes-in-c)
   - [4.2 Creating Custom Attributes & Abilities](#42-creating-custom-attributes--abilities)

---

## 1. Mover Framework Architecture

### 1.1 Overview & Legacy CharacterMovement Comparison

In traditional Unreal Engine gameplay architecture, character locomotion is governed by `UCharacterMovementComponent`. While historically functional, `CharacterMovementComponent` presents significant architectural limitations:

* **Monolithic Coupling:** Locomotion logic, collision handling, and networking are tightly coupled inside a single monolithic C++ class containing over 10,000 lines of code.
* **Class Rigidity:** It is hardcoded to inherit specifically from `ACharacter`, making it unusable for arbitrary `APawn` actors, non-humanoid physics, or custom vehicle systems without heavy hacks.
* **Inflexible Network Protocol:** The replication protocol is hardcoded to specific movement states (`MOVE_Walking`, `MOVE_Falling`, `MOVE_Swimming`), making custom movement modes difficult to synchronize reliably over high-latency networks.

Unreal Engine's **Mover** framework (`UCharacterMoverComponent`) solves these structural defects by decoupling physical movement simulation into modular **Movement Modes** and delegating networked state synchronization to Epic Games' standalone **Network Prediction** plugin.

| Architectural Feature | Legacy `CharacterMovementComponent` | Modern `Mover` Framework |
| :--- | :--- | :--- |
| **Object Hierarchy** | Monolithic class bound exclusively to `ACharacter`. | Modular component attachable to any `APawn`. |
| **Movement Logic** | Built-in monolithic state machine. | Pluggable `UBaseMovementMode` C++ sub-objects. |
| **Network Replication** | Legacy custom RPC and saved move buffer. | Delegated to generic **Network Prediction** plugin. |
| **Shared State Memory** | Scattered member variables across actor classes. | Structured blackboard (`UMoverBlackboard`). |
| **Constraint Solver** | Direct position offset translations. | Modular kinematic constraint solver. |

### 1.2 Movement Modes & Blackboard Memory

The Mover framework breaks down physical locomotion into discrete, self-contained movement modes inheriting from `UBaseMovementMode`. A pawn can dynamically register or unregister movement modes at runtime.

#### Blackboard Memory (`UMoverBlackboard`)

To prevent state pollution across actor headers, Mover utilizes a dedicated blackboard instance (`UMoverBlackboard`) accessible during simulation ticks. Volatile data—such as current movement base pointers, relative base transforms, and inertia accumulators—are read and written via typed blackboard keys.

For example, when a pawn attaches to or grabs an actor resting on a moving platform, resetting the dynamic movement base in the blackboard prevents unwanted inertia inheritance:

```cpp
if (UCharacterMoverComponent* MoverComp = Pawn->FindComponentByClass<UCharacterMoverComponent>())
{
	if (UMoverBlackboard* SimBlackboard = MoverComp->GetSimBlackboard_Mutable())
	{
		FRelativeBaseInfo EmptyBaseInfo;
		// Resets dynamic movement base to prevent unexpected physics velocity inheritance
		SimBlackboard->Set(CommonBlackboard::LastFoundDynamicMovementBase, EmptyBaseInfo);
	}
}
```

### 1.3 Input Command Context & Network Prediction

Mover simulation advances deterministically using an immutable per-tick input container struct: `FMoverInputCmdContext`.

#### Input Production Pipeline

Instead of directly altering velocity vectors upon key presses, player controllers or AI controllers feed movement intentions through the `IMoverInputProducerInterface` interface. The pawn implements `ProduceInput_Implementation`, which constructs an `FMoverInputCmdContext` each tick.

```
+------------------------------------+
|  Human Key Input / AI Navigation   |
+------------------------------------+
                  |
                  v
+------------------------------------+
|    AGSPawn::ProduceInput()         |
+------------------------------------+
                  |
                  v  Populates FCharacterDefaultInputs
+------------------------------------+
|      FMoverInputCmdContext         |
+------------------------------------+
       |                     |
       v Local Simulation    v Sent via UDP
+--------------+     +------------------------+
| Local Solver |     | Network Prediction     |
| (Predictive) |     | Buffer (Server Bound)  |
+--------------+     +------------------------+
```

#### Code Implementation (`AGSPawn::ProduceInput_Implementation`)

```cpp
void AGSPawn::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	// 1. Obtain mutable input collection for standard character inputs
	FCharacterDefaultInputs& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();
	FVector MoveDirection = FVector::ZeroVector;

	// 2. AI Navigation Pathing Input
	if (NavMoverComponent)
	{
		FVector NavMoveInputIntent = FVector::ZeroVector;
		FVector NavMoveInputVelocity = FVector::ZeroVector;
		if (NavMoverComponent->ConsumeNavMovementData(NavMoveInputIntent, NavMoveInputVelocity))
		{
			MoveDirection = NavMoveInputIntent.IsNearlyZero() ? NavMoveInputVelocity.GetSafeNormal() : NavMoveInputIntent;
		}
	}

	// 3. Human Analog Locomotion Input (Camera-Oriented)
	if (MoveDirection.IsNearlyZero() && !CachedMovementInput.IsZero())
	{
		FRotator BaseRotation = FRotator::ZeroRotator;
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			BaseRotation = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation() : PC->GetControlRotation();
		}
		const FRotator YawRotation(0.f, BaseRotation.Yaw, 0.f);
		const FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		MoveDirection = (ForwardVector * CachedMovementInput.Y) + (RightVector * CachedMovementInput.X);
		MoveDirection.Normalize();
	}

	// Assign direction intent and jump flags to Mover context
	CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, MoveDirection);
	CharacterInputs.bIsJumpPressed = bCachedJumpPressed;
	CharacterInputs.bIsJumpJustPressed = bCachedJumpJustPressed;
	CharacterInputs.OrientationIntent = MoveDirection;
	CharacterInputs.ControlRotation = GetControlRotation();

	bCachedJumpJustPressed = false;
}
```

### 1.4 Reconciliation, Rollback, and Replay

When a client simulates movement locally ahead of the server, it records input contexts in a circular buffer managed by the Network Prediction plugin.

1. **Authoritative Server Execution:** The server processes client inputs and broadcasts an authoritative state struct (`FMoverSyncState`).
2. **Reconciliation Check:** The client compares its historic local state against the server state via `FMoverSyncState::ShouldReconcile`.
3. **Rollback & Replay:** If a mismatch is detected (e.g., due to packet loss, obstacle collision, or speed modification):
   - The local client state is immediately restored to the authoritative server position (**Rollback**).
   - Unacknowledged client inputs stored in the local buffer are re-simulated sequentially in a single frame tick (**Replay**), bringing the client back to present time seamlessly.

### 1.5 Mover vs. Chaos Physics Coexistence

Unreal Engine's **Chaos Physics** engine executes dynamic rigid body simulations asynchronously across sub-threads (**Physics Substepping**).

While Chaos provides excellent dynamic ragdolls and destructibles, its asynchronous substepping presents a fundamental conflict with Network Prediction:
* **Non-Deterministic Rollbacks:** When Mover rewinds client transform states during a network reconciliation, Chaos cannot rewind physical rigid body velocities synchronously on the exact frame tick. This leads to physical jitter, velocity accumulation errors, and visual snapping in multiplayer settings.

#### Kinematic Solution (`LaunchKinematic`)

To achieve smooth multiplayer synchronization for thrown objects or physical manipulation without physics desynchronization, interactive objects bypass free dynamic Chaos physics during flight. Instead, motion is calculated explicitly via step-by-step kinematic parabolic interpolation (`LaunchKinematic`), ensuring complete determinism during Mover rollbacks.

---

## 2. Bidirectional Mover + GAS Integration

A core strength of this architecture is the real-time, bidirectional coupling between the Gameplay Ability System (GAS) and the Mover framework.

```
+------------------------------------+
|  Gameplay Ability System (GAS)     |
|  - AttributeSets (WalkSpeed)       |
|  - GameplayTags (State.Aiming)     |
+------------------------------------+
       ^                      |
       | Ability Cancel       | Delegate Callback
       | Signal               v (OnWalkSpeedChanged)
+------------------------------------+
|  UCharacterMoverComponent          |
|  - MaxSpeed Settings Update        |
|  - Input Orientation Overrides     |
+------------------------------------+
```

### 2.1 Reactive Attribute Binding (WalkSpeed)

Rather than executing costly polling routines inside `Tick` to query character speed attributes, the pawn registers numerical change delegates with the Ability System Component (ASC) during setup (`InitAbilityActorInfo`).

#### Delegate Registration & Listener Implementation

```cpp
void AGSPawn::InitAbilityActorInfo()
{
	Super::InitAbilityActorInfo();

	if (AbilitySystemComponent && MovementSet)
	{
		// Unsubscribe prior listeners to prevent duplicate execution
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			MovementSet->GetWalkSpeedAttribute()).RemoveAll(this);

		// Register reactive listener for WalkSpeed attribute updates
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			MovementSet->GetWalkSpeedAttribute()).AddUObject(this, &AGSPawn::OnWalkSpeedChanged);

		// Synchronize initial physical max speed setting in Mover
		if (MoverComponent)
		{
			if (UCommonLegacyMovementSettings* MoveSettings = MoverComponent->FindSharedSettings_Mutable<UCommonLegacyMovementSettings>())
			{
				MoveSettings->MaxSpeed = MovementSet->GetWalkSpeed();
			}
		}
	}
}

void AGSPawn::OnWalkSpeedChanged(const FOnAttributeChangeData& Data)
{
	if (MoverComponent)
	{
		// Dynamically update maximum physical speed constraint in Mover
		if (UCommonLegacyMovementSettings* MoveSettings = MoverComponent->FindSharedSettings_Mutable<UCommonLegacyMovementSettings>())
		{
			MoveSettings->MaxSpeed = Data.NewValue;
		}
	}
}
```

### 2.2 Locomotion Input & Ability Cancellation

When a player performs locomotion while executing passive abilities (e.g., emotes), Mover input production inspects active GAS tags and triggers ability cancellations instantly upon motion detection.

```cpp
if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(GSGameplayTags::State_Emoting))
{
	if (!CachedMovementInput.IsZero() || bCachedJumpPressed)
	{
		FGameplayTagContainer EmotingTags;
		EmotingTags.AddTag(GSGameplayTags::State_Emoting);
		// Locomotion input detected: cancel active Emote abilities
		AbilitySystemComponent->CancelAbilities(&EmotingTags);
	}
}
```

### 2.3 Kinematic Trajectory Simulation & Stacking Equations

When manipulating or carrying objects, vertical offset distances must be calculated dynamically based on physical collision bounds.

#### Vertical Item Stacking Height Equation

$$Offset_Z = BaseOffset + InitialOffset + \sum_{i=1}^{StackCount} Height_i + (StackCount \times SpacingOffset)$$

#### Stacking Height Implementation (`GSAbility_Grab`)

```cpp
float CumulativeHeight = 0.0f;
int32 StackCount = 0;
TArray<AActor*> CurrAttached;
AvatarActor->GetAttachedActors(CurrAttached);

for (AActor* AttachedActor : CurrAttached)
{
	if (AttachedActor && AttachedActor != TargetItem)
	{
		UGSGrabbableComponent* TempGrabComp = AttachedActor->FindComponentByClass<UGSGrabbableComponent>();
		if (TempGrabComp && TempGrabComp->IsGrabbed())
		{
			float ItemHeight = DefaultItemHeight;
			if (bUseDynamicBoundsHeight)
			{
				FVector Origin, BoxExtent;
				AttachedActor->GetActorBounds(false, Origin, BoxExtent);
				float CalculatedHeight = BoxExtent.Z * 2.0f;
				if (CalculatedHeight > 0.0f)
				{
					ItemHeight = CalculatedHeight;
				}
			}
			CumulativeHeight += ItemHeight;
			StackCount++;
		}
	}
}
```

---

## 3. Verse & Unreal Engine 6.0 Paradigm Shift

Unreal Engine 6.0 introduces **Verse** as a core native programming language. The architectural choices in this C++ framework were designed specifically to streamline future migration to Verse paradigms.

### 3.1 Reactive Input Streams vs. Polling Cycles

In C++, Mover polls hardware input states every tick to populate `FMoverInputCmdContext`. In Verse, polling loops are replaced by asynchronous event streams (`event`).

```verse
# Conceptual Verse Architecture for Mover Input (UE 6.0)
LocomotionStream := channel(vector3){}

OnInputReceived(MoveIntent : vector3) : void =
    spawn:
        LocomotionStream.Send(MoveIntent)

ListenToLocomotion()<suspends> : void =
    loop:
        Intent := LocomotionStream.Receive()
        MoverComponent.ApplyDirectionalIntent(Intent)
```

### 3.2 Software Transactional Memory (STM) Reconciliation

Network prediction rollbacks in C++ require explicit comparison functions (`ShouldReconcile`). In Verse, prediction leverage **Software Transactional Memory (STM)**.

Transactions execute speculatively. If network desynchronization occurs, the runtime aborts the transaction block and reverts memory state atomically without custom C++ rewind routines.

### 3.3 Concurrent Async Coroutines for Physics

Complex state machines governing abilities, throwing arcs, or patience timers rely on `FTimerHandle` in C++. In Verse, concurrent control flow primitives (`race`, `sync`, `branch`) eliminate timer handle boilerplate entirely.

```verse
# Conceptual Verse Race Condition for NPC Patience & Delivery
RaceNPCPatience(PatienceSeconds : float)<suspends> : void =
    race:
        block:
            ItemDelivered := AwaitFoodDelivery()
            ProcessPayment(ItemDelivered)
        block:
            Sleep(PatienceSeconds)
            DepartEnraged()
```

---

## 4. Extending the Framework

### 4.1 Creating Custom Movement Modes in C++

1. Create a C++ class inheriting from `UBaseMovementMode`.
2. Override `OnGenerateMove` and `OnSimulationTick`.
3. Register the mode class in `UCharacterMoverComponent` settings inside your Pawn constructor.

```cpp
UCLASS(BlueprintType)
class PROJECTF_API UGSBaseCustomMovementMode : public UBaseMovementMode
{
	GENERATED_BODY()
public:
	virtual void OnSimulationTick(const FMoverTimeStep& TimeStep, const FMoverInputCmdContext& InputCmd, FMoverSyncState& OutputSyncState) override;
};
```

### 4.2 Creating Custom Attributes & Abilities

1. Inherit from `UAttributeSet` and define attributes using macro helpers (`ATTRIBUTE_ACCESSORS`).
2. Add attribute set classes to item or pawn data assets.
3. Inherit from `UGameplayAbility` to create active abilities reacting to tags.
