# Mover Component & Gameplay Ability System (GAS) Technical Framework Guide

Welcome to the technical reference manual for the modular movement, physical simulation, and Gameplay Ability System (GAS) framework in Unreal Engine 5.8.

This document provides a line-by-line architectural breakdown explaining **how** systems function, **why** specific C++ macros, types, and architectural decisions were made, and how upcoming migration paths map toward Verse in Unreal Engine 6.0.

---

## Author's Notes: The Paradigm Shift in Game Architecture

> [!NOTE]
> **A Note on Why We Are Moving Away from Legacy OOP**
> 
> If you have worked in game development for any length of time, you have likely felt the pain of traditional Object-Oriented Programming (OOP). In standard engine workflows, creating a new gameplay entity meant subclassing `ACharacter` or `AActor`, overloading virtual functions, scattering member variables across header files, and polling booleans inside `Tick`.
> 
> As projects scale, this legacy approach creates three massive bottlenecks:
> 1. **Inheritance Rigidity:** Monolithic classes like legacy `CharacterMovementComponent` tie locomotion exclusively to humanoids. Extending it for custom physics or vehicles requires fighting thousands of lines of base code.
> 2. **Polling Overhead & CPU Cache Misses:** Polling states every frame inside `Tick` wastes CPU cycles. Scattering data across memory pointers creates cache misses on modern hardware.
> 3. **High Iteration Friction:** Requiring C++ re-compilations or Blueprint subclassing for simple asset changes slows down design iteration.
> 
> Game development across modern engines is shifting toward **Data-Driven Architecture (POD/DOD)** and **Asynchronous Reactive Concurrency** (such as Verse in UE 6.0 or Luau in Roblox).
> 
> I studied and implemented the complex C++ architecture behind Mover, GAS, Network Prediction, StateTree, and Verse specifically to make game creation effortless—so that creators can focus entirely on imagination rather than fighting technical debt. This document details the exact engineering behind that shift.

---

## Table of Contents

1. [Mover Framework Architecture](#1-mover-framework-architecture)
   - [1.1 Overview & Legacy CharacterMovement Comparison](#11-overview--legacy-charactermovement-comparison)
   - [1.2 Pawn Initialization & Component Breakdown](#12-pawn-initialization--component-breakdown)
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

In traditional Unreal Engine gameplay architecture, character locomotion is governed by `UCharacterMovementComponent`. While historically functional, `CharacterMovementComponent` presents structural defects:

* **Monolithic Coupling:** Locomotion logic, collision handling, and networking are tightly coupled inside a single monolithic C++ class containing over 10,000 lines of code.
* **Class Rigidity:** It is hardcoded to inherit specifically from `ACharacter`, making it unusable for arbitrary `APawn` actors, non-humanoid physics, or custom vehicle systems without heavy workarounds.
* **Inflexible Network Protocol:** The replication protocol is hardcoded to specific movement states (`MOVE_Walking`, `MOVE_Falling`, `MOVE_Swimming`), making custom movement modes difficult to synchronize reliably over high-latency networks.

Unreal Engine's **Mover** framework (`UCharacterMoverComponent`) solves these structural defects by decoupling physical movement simulation into modular **Movement Modes** and delegating networked state synchronization to Epic Games' standalone **Network Prediction** plugin.

> [!NOTE]
> **Architectural Advantage of Mover**
> Unlike legacy `CharacterMovementComponent` which requires inheriting from `ACharacter`, Mover attaches directly to any base `APawn`. This allows pawns to remain lightweight and decoupled from character-specific legacy code.

| Architectural Feature | Legacy `CharacterMovementComponent` | Modern `Mover` Framework |
| :--- | :--- | :--- |
| **Object Hierarchy** | Monolithic class bound exclusively to `ACharacter`. | Modular component attachable to any `APawn`. |
| **Movement Logic** | Built-in monolithic state machine. | Pluggable `UBaseMovementMode` C++ sub-objects. |
| **Network Replication** | Legacy custom RPC and saved move buffer. | Delegated to generic **Network Prediction** plugin. |
| **Shared State Memory** | Scattered member variables across actor classes. | Structured blackboard (`UMoverBlackboard`). |
| **Constraint Solver** | Direct position offset translations. | Modular kinematic constraint solver. |

---

### 1.2 Pawn Initialization & Component Breakdown

To understand how Mover initializes physical simulation, consider the component declarations in `AGSPawn.h`:

```cpp
protected:
	/** Capsule component managing collision bounds in the world */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	/** Core Mover component resolving pawn physics simulation tick */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCharacterMoverComponent> MoverComponent;

	/** Navigation component translating AI pathing intentions to Mover */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNavMoverComponent> NavMoverComponent;
```

#### Why are components declared this way?

* `TObjectPtr<T>`: Used instead of raw C++ pointers (`T*`). In Unreal Engine 5+, `TObjectPtr` provides build-time garbage collection tracking, access tracking in development builds, and seamless integration with Unreal's object memory model.
* `VisibleAnywhere, BlueprintReadOnly`: Exposes component references to Blueprints for inspection without allowing designer scripts to overwrite pointer addresses at runtime.

Now examine the constructor implementation in `AGSPawn.cpp`:

```cpp
AGSPawn::AGSPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. Initialize root collision capsule
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	CapsuleComponent->InitCapsuleSize(35.f, 90.f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	CapsuleComponent->SetShouldUpdatePhysicsVolume(true);
	CapsuleComponent->SetCanEverAffectNavigation(true);
	CapsuleComponent->bDynamicObstacle = true;
	RootComponent = CapsuleComponent;

	// 2. Instantiate modern movement components
	MoverComponent = CreateDefaultSubobject<UCharacterMoverComponent>(TEXT("MoverComponent"));
	NavMoverComponent = CreateDefaultSubobject<UNavMoverComponent>(TEXT("NavMoverComponent"));

	// 3. CRITICAL NETWORKING DESIGN DECISION
	SetReplicateMovement(false);
	bReplicates = true;
}
```

> [!IMPORTANT]
> **Why `SetReplicateMovement(false)` is Required**
> In standard Unreal Engine networking, `SetReplicateMovement(true)` instructs the engine to replicate actor transforms directly via legacy actor channels.
> Because Mover delegates physical locomotion replication to the **Network Prediction** plugin, standard actor movement replication MUST be disabled (`SetReplicateMovement(false)`). Enabling both causes replication feedback loops, visual jitter, and bandwidth duplication.

---

### 1.3 Input Command Context & Network Prediction

Mover simulation advances deterministically using an immutable per-tick input container struct: `FMoverInputCmdContext`.

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

The pawn implements `IMoverInputProducerInterface` via `ProduceInput_Implementation`. This function runs every tick to package player and AI inputs:

```cpp
void AGSPawn::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	// 1. Obtain mutable input collection container for default character inputs
	FCharacterDefaultInputs& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();
	FVector MoveDirection = FVector::ZeroVector;

	// 2. AI Navigation Pathing Route
	if (NavMoverComponent)
	{
		FVector NavMoveInputIntent = FVector::ZeroVector;
		FVector NavMoveInputVelocity = FVector::ZeroVector;
		if (NavMoverComponent->ConsumeNavMovementData(NavMoveInputIntent, NavMoveInputVelocity))
		{
			// If directional intent is zero, fall back to current pathfinding velocity vector
			MoveDirection = NavMoveInputIntent.IsNearlyZero() ? NavMoveInputVelocity.GetSafeNormal() : NavMoveInputIntent;
		}
	}

	// 3. Human Analog Locomotion Route (Camera-Oriented Projection)
	if (MoveDirection.IsNearlyZero() && !CachedMovementInput.IsZero())
	{
		FRotator BaseRotation = FRotator::ZeroRotator;
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			BaseRotation = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation() : PC->GetControlRotation();
		}
		
		// Zero out Pitch and Roll to project movement purely onto horizontal XY plane
		const FRotator YawRotation(0.f, BaseRotation.Yaw, 0.f);
		const FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		MoveDirection = (ForwardVector * CachedMovementInput.Y) + (RightVector * CachedMovementInput.X);
		MoveDirection.Normalize();
	}

	// 4. Package movement direction and jump states into input context
	CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, MoveDirection);
	CharacterInputs.bIsJumpPressed = bCachedJumpPressed;
	CharacterInputs.bIsJumpJustPressed = bCachedJumpJustPressed;

	// 5. Orientation Intent: GAS Aiming Overrides vs. Locomotion Direction
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
		CharacterInputs.OrientationIntent = MoveDirection;
	}

	CharacterInputs.ControlRotation = GetControlRotation();
	bCachedJumpJustPressed = false;
}
```

#### Detailed Code Walkthrough

1. **Why `FindOrAddMutableDataByType<FCharacterDefaultInputs>()`?**
   Mover uses generic polymorphic collections (`FMoverDataCollection`). This template method retrieves or instantiates structured character input data (`FCharacterDefaultInputs`) without tight type coupling.
2. **Why zero out Pitch and Roll in `FRotator(0.f, BaseRotation.Yaw, 0.f)`?**
   When players look up or down, the camera's forward vector tilts toward the sky or ground. If pitch was included, pushing forward on the analog stick would cause characters to move into the ground or fly upward. Isolating `Yaw` guarantees horizontal ground movement.
3. **Why check `State_Aiming` tag for mouse raycasting?**
   When players aim to throw items, character rotation must decouple from movement direction. The code performs a mouse cursor raycast (`GetHitResultUnderCursor`), calculates the 2D aim vector, and sets `CharacterInputs.OrientationIntent` independently of `MoveDirection`.

---

### 1.4 Reconciliation, Rollback, and Replay

When a client simulates movement locally ahead of the server, it records input contexts in a circular buffer managed by the Network Prediction plugin.

```cpp
bool FMoverSyncState::ShouldReconcile(const FMoverSyncState& AuthorityState) const
{
	return (MovementMode != AuthorityState.MovementMode) || 
	       SyncStateCollection.ShouldReconcile(AuthorityState.SyncStateCollection) ||
	       MovementModifiers.ShouldReconcile(AuthorityState.MovementModifiers);
}
```

#### The Reconciliation Sequence:

1. **Authoritative Server Execution:** The server processes client inputs and broadcasts an authoritative state struct (`FMoverSyncState`).
2. **Reconciliation Check:** The client compares its historic local state against the server state via `FMoverSyncState::ShouldReconcile`.
3. **Rollback & Replay:** If a mismatch is detected:
   - The local client state is restored to the authoritative server position (**Rollback**).
   - Unacknowledged client inputs stored in the local buffer are re-simulated sequentially in a single frame tick (**Replay**), bringing the client back to present time seamlessly.

> [!TIP]
> **Why Determinism Matters in Replay**
> During replay, Mover re-simulates tens of frames in milliseconds. If any sub-system modified non-deterministic random state during simulation ticks, reconciliation would fail repeatedly, creating visual stutter.

---

### 1.5 Mover vs. Chaos Physics Coexistence

Unreal Engine's **Chaos Physics** engine executes dynamic rigid body simulations asynchronously across sub-threads (**Physics Substepping**).

While Chaos provides dynamic ragdolls and destructibles, its asynchronous substepping presents a fundamental conflict with Network Prediction:
* **Non-Deterministic Rollbacks:** When Mover rewinds client transform states during a network reconciliation, Chaos cannot rewind physical rigid body velocities synchronously on the exact frame tick. This leads to physical jitter, velocity accumulation errors, and visual snapping in multiplayer settings.

#### Kinematic Solution (`LaunchKinematic`)

To achieve smooth multiplayer synchronization for thrown objects without physics desynchronization, interactive objects bypass free dynamic Chaos physics during flight. Instead, motion is calculated explicitly via step-by-step kinematic parabolic interpolation (`LaunchKinematic`), ensuring complete determinism during Mover rollbacks.

---

## 2. Bidirectional Mover + GAS Integration

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

```cpp
void AGSPawn::InitAbilityActorInfo()
{
	Super::InitAbilityActorInfo();

	if (AbilitySystemComponent && MovementSet)
	{
		// 1. Remove prior delegate subscriptions to prevent memory leaks or duplicate calls
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			MovementSet->GetWalkSpeedAttribute()).RemoveAll(this);

		// 2. Register reactive listener for WalkSpeed attribute updates
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			MovementSet->GetWalkSpeedAttribute()).AddUObject(this, &AGSPawn::OnWalkSpeedChanged);

		// 3. Synchronize initial physical max speed setting in Mover
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
		// Dynamically update maximum physical speed constraint in Mover settings
		if (UCommonLegacyMovementSettings* MoveSettings = MoverComponent->FindSharedSettings_Mutable<UCommonLegacyMovementSettings>())
		{
			MoveSettings->MaxSpeed = Data.NewValue;
		}
	}
}
```

#### Detailed Code Walkthrough

* **Why use delegates instead of `Tick`?**
  Polling attributes every frame wastes CPU cycles, especially in multiplayer sessions with dozens of pawns. Delegate listeners execute code **only** when a Gameplay Effect modifies `WalkSpeed`, providing zero-cost performance when speed is constant.
* **Why `FindSharedSettings_Mutable<UCommonLegacyMovementSettings>()`?**
  Mover encapsulates movement parameters inside shared settings objects. This function retrieves mutable access to the legacy settings container, allowing max movement speed updates on the fly.

---

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

---

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
				// Query physical collision bounds of attached item
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

In C++, Mover polls hardware input states every tick to populate `FMoverInputCmdContext`. In Verse, polling loops are replaced by asynchronous event streams (`channel`).

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

> [!NOTE]
> **Key Difference in Verse**
> In Verse, functions marked `<suspends>` can pause execution without blocking OS threads. This makes managing asynchronous user input streams cleaner and safer than C++ tick callbacks.

---

### 3.2 Software Transactional Memory (STM) Reconciliation

Network prediction rollbacks in C++ require explicit comparison functions (`ShouldReconcile`). In Verse, prediction leverages **Software Transactional Memory (STM)**.

Transactions execute speculatively. If network desynchronization occurs, the runtime aborts the transaction block and reverts memory state atomically without custom C++ rewind routines.

---

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

---

### 4.2 Creating Custom Attributes & Abilities

1. Inherit from `UAttributeSet` and define attributes using macro helpers (`ATTRIBUTE_ACCESSORS`).
2. Add attribute set classes to item or pawn data assets.
3. Inherit from `UGameplayAbility` to create active abilities reacting to tags.
