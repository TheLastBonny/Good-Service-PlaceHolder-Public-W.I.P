# Mover Component & Gameplay Ability System (GAS) Technical Framework Guide

Welcome to the master technical reference manual for the modular movement, physical simulation, and Gameplay Ability System (GAS) framework in Unreal Engine 5.8.

This document serves as the complete, un-truncated architectural guide—modeled after community reference standards such as Tranek's GAS Documentation—providing full C++ source snippets, line-by-line code breakdowns, mathematical equations, memory layout explanations, and Verse migration patterns for Unreal Engine 6.0.

---

## Table of Contents

1. [Author's Notes: The Paradigm Shift in Game Architecture](#1-authors-notes-the-paradigm-shift-in-game-architecture)
2. [Pawn Architecture & Component Breakdown](#2-pawn-architecture--component-breakdown)
   - [2.1 Header Declarations & Pointer Safety (`AGSPawn.h`)](#21-header-declarations--pointer-safety-agspawnh)
   - [2.2 Constructor Setup & Replication Rules (`AGSPawn.cpp`)](#22-constructor-setup--replication-rules-agspawncpp)
3. [Networked Kinematic Input Context (`FMoverInputCmdContext`)](#3-networked-kinematic-input-context-fmoverinputcmdcontext)
   - [3.1 Input Production Pipeline](#31-input-production-pipeline)
   - [3.2 Implementation & Line-by-Line Code Analysis](#32-implementation--line-by-line-code-analysis)
4. [Mover Simulation States, Reconciliation & Physics Coexistence](#4-mover-simulation-states-reconciliation--physics-coexistence)
   - [4.1 `FMoverSyncState` & Reconciliation Checks](#41-fmoversyncstate--reconciliation-checks)
   - [4.2 Frame Tick Transition States (`FMovementModeTickEndState`)](#42-frame-tick-transition-states-fmovementmodetickendstate)
   - [4.3 Mover vs. Chaos Physics Coexistence (`LaunchKinematic`)](#43-mover-vs-chaos-physics-coexistence-launchkinematic)
5. [Bidirectional Mover + GAS Integration](#5-bidirectional-mover--gas-integration)
   - [5.1 Reactive Attribute Change Delegates (`WalkSpeed`)](#51-reactive-attribute-change-delegates-walkspeed)
   - [5.2 Locomotion Input & Ability Cancellation](#52-locomotion-input--ability-cancellation)
   - [5.3 Vertical Item Stacking Equations & Dynamic Bounds](#53-vertical-item-stacking-equations--dynamic-bounds)
6. [Data-Driven Item Architecture (`AGSItem` & `UGSItemDataAsset`)](#6-data-driven-item-architecture-agsitem--ugsitemdataasset)
   - [6.1 Data Asset Definition (`GSItemDataAsset.h`)](#61-data-asset-definition-gsitemdataasseth)
   - [6.2 Action Execution Logic (`GSItemDataAsset.cpp`)](#62-action-execution-logic-gsitemdataassetcpp)
   - [6.3 Event Listener Binding (`GSItem.cpp`)](#63-event-listener-binding-gsitemcpp)
7. [Interactive Utility Stations (`AGSUtilityStation`)](#7-interactive-utility-stations-agsutilitystation)
   - [7.1 Station Processing Engine (`UpdateEffectsForItem`)](#71-station-processing-engine-updateeffectsforitem)
   - [7.2 Socket Attachment & Collision Snapping](#72-socket-attachment--collision-snapping)
8. [Networked Emotes & 3D Spatialized Audio](#8-networked-emotes--3d-spatialized-audio)
   - [8.1 Multicast RPC Sound Spawning](#81-multicast-rpc-sound-spawning)
9. [NPC Customer Logic & Delivery Validation](#9-npc-customer-logic--delivery-validation)
   - [9.1 Order Match & Cooked/Burned Validation](#91-order-match--cookedburned-validation)
   - [9.2 Currency Spawning on Eating Completion](#92-currency-spawning-on-eating-completion)
10. [StateTree C++ Task Implementations](#10-statetree-c-task-implementations)
    - [10.1 Table Assignment Task (`GSStateTreeTask_AssignTable`)](#101-table-assignment-task-gsstatetreetask_assigntable)
    - [10.2 Order Selection Task (`GSStateTreeTask_ChooseRandomOrder`)](#102-order-selection-task-gsstatetreetask_chooserandomorder)
    - [10.3 Send To Exit Task (`GSStateTreeTask_SendToExit`)](#103-send-to-exit-task-gsstatetreetask_sendtoexit)
11. [Verse & Unreal Engine 6.0 Paradigm Shift](#11-verse--unreal-engine-60-paradigm-shift)
    - [11.1 Reactive Input Streams vs. Polling Loops](#111-reactive-input-streams-vs-polling-loops)
    - [11.2 Software Transactional Memory (STM) Reconciliation](#112-software-transactional-memory-stm-reconciliation)
    - [11.3 Concurrent Async Coroutines for Mechanics](#113-concurrent-async-coroutines-for-mechanics)
12. [Extending the Framework](#12-extending-the-framework)
    - [12.1 Writing Custom Movement Modes in C++](#121-writing-custom-movement-modes-in-c)
    - [12.2 Writing Custom Attribute Sets & GAS Abilities](#122-writing-custom-attribute-sets--gas-abilities)

---

## 1. Author's Notes: The Paradigm Shift in Game Architecture

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

## 2. Pawn Architecture & Component Breakdown

### 2.1 Header Declarations & Pointer Safety (`AGSPawn.h`)

The primary pawn `AGSPawn` encapsulates modern movement components in its header file:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemInterface.h"
#include "MoverComponent.h"
#include "NavMoverComponent.h"
#include "Components/CapsuleComponent.h"
#include "GSPawn.generated.h"

UCLASS(BlueprintType)
class PROJECTF_API AGSPawn : public APawn, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGSPawn();

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
};
```

#### Detailed Code Breakdown:

* `TObjectPtr<T>`: Replaces raw C++ pointers (`T*`). Introduced in UE5, `TObjectPtr` provides automatic build-time garbage collection tracking, optional access tracking in development builds, and seamless integration with Unreal's object memory model.
* `VisibleAnywhere, BlueprintReadOnly`: Exposes component sub-objects to Blueprint inspection windows while preventing designer scripts from overwriting pointer references at runtime.

---

### 2.2 Constructor Setup & Replication Rules (`AGSPawn.cpp`)

```cpp
#include "Characters/GSPawn.h"

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

## 3. Networked Kinematic Input Context (`FMoverInputCmdContext`)

### 3.1 Input Production Pipeline

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

---

### 3.2 Implementation & Line-by-Line Code Analysis

The pawn implements `IMoverInputProducerInterface` via `ProduceInput_Implementation`. This function executes every frame tick to package movement input data:

```cpp
void AGSPawn::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	// 1. Cancel passive abilities (Emotes) upon motion input
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(GSGameplayTags::State_Emoting))
	{
		if (!CachedMovementInput.IsZero() || bCachedJumpPressed)
		{
			FGameplayTagContainer EmotingTags;
			EmotingTags.AddTag(GSGameplayTags::State_Emoting);
			AbilitySystemComponent->CancelAbilities(&EmotingTags);
		}
	}

	// 2. Obtain mutable input collection container for default character inputs
	FCharacterDefaultInputs& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();
	FVector MoveDirection = FVector::ZeroVector;

	// 3. AI Navigation Pathing Route
	if (NavMoverComponent)
	{
		FVector NavMoveInputIntent = FVector::ZeroVector;
		FVector NavMoveInputVelocity = FVector::ZeroVector;
		if (NavMoverComponent->ConsumeNavMovementData(NavMoveInputIntent, NavMoveInputVelocity))
		{
			// Fallback to pathfinding velocity vector if directional intent is zero
			if (NavMoveInputIntent.IsNearlyZero() && !NavMoveInputVelocity.IsNearlyZero())
			{
				MoveDirection = NavMoveInputVelocity.GetSafeNormal();
			}
			else
			{
				MoveDirection = NavMoveInputIntent;
			}
		}
	}

	// 4. Human Analog Locomotion Route (Camera-Oriented Projection)
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
		
		// Zero out Pitch and Roll to project movement purely onto horizontal XY plane
		const FRotator YawRotation(0.f, BaseRotation.Yaw, 0.f);
		const FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		MoveDirection = (ForwardVector * CachedMovementInput.Y) + (RightVector * CachedMovementInput.X);
		MoveDirection.Normalize();
	}

	// 5. Package movement direction and jump states into input context
	CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, MoveDirection);
	CharacterInputs.bIsJumpPressed = bCachedJumpPressed;
	CharacterInputs.bIsJumpJustPressed = bCachedJumpJustPressed;

	// 6. Orientation Intent: GAS Aiming Overrides vs. Locomotion Direction
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

## 4. Mover Simulation States, Reconciliation & Physics Coexistence

### 4.1 `FMoverSyncState` & Reconciliation Checks

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

---

### 4.2 Frame Tick Transition States (`FMovementModeTickEndState`)

At the end of a movement mode tick, Mover returns an `FMovementModeTickEndState` struct containing transition details:

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

#### Why `RemainingMs` is Essential:
If a character walks off a cliff halfway through a 16ms simulation tick, walking mode ends after 8ms, returns `RemainingMs = 8.0f`, and Mover immediately activates `Falling` mode to consume the remaining 8ms in the exact same physics frame.

---

### 4.3 Mover vs. Chaos Physics Coexistence (`LaunchKinematic`)

Unreal Engine's **Chaos Physics** engine executes dynamic rigid body simulations asynchronously across sub-threads (**Physics Substepping**).

While Chaos provides dynamic ragdolls and destructibles, its asynchronous substepping presents a fundamental conflict with Network Prediction:
* **Non-Deterministic Rollbacks:** When Mover rewinds client transform states during a network reconciliation, Chaos cannot rewind physical rigid body velocities synchronously on the exact frame tick. This leads to physical jitter, velocity accumulation errors, and visual snapping in multiplayer settings.

#### Kinematic Solution (`LaunchKinematic`)

To achieve smooth multiplayer synchronization for thrown objects without physics desynchronization, interactive objects bypass free dynamic Chaos physics during flight. Instead, motion is calculated explicitly via step-by-step kinematic parabolic interpolation (`LaunchKinematic`) in `UGSGrabbableComponent`:

```cpp
// Scaled throwing velocity and parabolic arc calculations in C++
float ScaledSpeed = FMath::Lerp(GrabComp->ThrowSpeed * 0.4f, GrabComp->ThrowSpeed, ClampedDistance / MaxThrowDistance);
float EstimatedPathLength = HorizontalDistance + (1.5f * LaunchZ);
float ThrowDuration = EstimatedPathLength / ScaledSpeed;

// Initiate deterministic kinematic parabolic launch
GrabComp->LaunchKinematic(LaunchStartLoc, DispersedTarget, LaunchZ, ThrowDuration);
```

---

## 5. Bidirectional Mover + GAS Integration

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

---

### 5.1 Reactive Attribute Change Delegates (`WalkSpeed`)

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

---

### 5.2 Locomotion Input & Ability Cancellation

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

### 5.3 Vertical Item Stacking Equations & Dynamic Bounds

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

## 6. Data-Driven Item Architecture (`AGSItem` & `UGSItemDataAsset`)

### 6.1 Data Asset Definition (`GSItemDataAsset.h`)

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AttributeSet.h"
#include "GSItemDataAsset.generated.h"

// Abstract base class for polymorphic actions associated with item state transitions
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTF_API UGSItemStateAction : public UObject
{
	GENERATED_BODY()
public:
	virtual void Execute(AActor* Owner) {}
};

// Action to override item's static mesh
UCLASS(BlueprintType, EditInlineNew, meta=(DisplayName="Mesh Override"))
class PROJECTF_API UGSItemStateAction_MeshOverride : public UGSItemStateAction
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> MeshOverride = nullptr;

	virtual void Execute(AActor* Owner) override;
};

// Action to play 3D audio effect
UCLASS(BlueprintType, EditInlineNew, meta=(DisplayName="Play Sound"))
class PROJECTF_API UGSItemStateAction_PlaySound : public UGSItemStateAction
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> SoundOverride = nullptr;

	virtual void Execute(AActor* Owner) override;
};

// Structure detailing item behavior for a specific state tag
USTRUCT(BlueprintType)
struct FGSItemStateDetails
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	FGameplayAttribute MaxProgressAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	float MaxProgressValue = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	FText StateName;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Actions")
	TArray<TObjectPtr<UGSItemStateAction>> Actions;
};

UCLASS(BlueprintType)
class PROJECTF_API UGSItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UGSItemDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Configuration")
	FGameplayTagContainer DefaultTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Configuration")
	TArray<TSubclassOf<UAttributeSet>> AttributeSets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Configuration")
	TMap<FGameplayTag, FGSItemStateDetails> ItemStatesMap;

	UFUNCTION(BlueprintCallable, Category = "Item")
	FGSItemStateDetails GetStateDetails(FGameplayTag StateTag) const;
};
```

---

### 6.2 Action Execution Logic (`GSItemDataAsset.cpp`)

```cpp
#include "DataAssets/GSItemDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

void UGSItemStateAction_MeshOverride::Execute(AActor* Owner)
{
	if (!Owner || !MeshOverride) return;

	UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Owner->GetComponentByClass(UStaticMeshComponent::StaticClass()));
	if (MeshComp)
	{
		MeshComp->SetStaticMesh(MeshOverride);
	}
}

void UGSItemStateAction_PlaySound::Execute(AActor* Owner)
{
	if (!Owner || !SoundOverride) return;
	if (Owner->GetNetMode() != NM_DedicatedServer)
	{
		UGameplayStatics::PlaySoundAtLocation(Owner, SoundOverride, Owner->GetActorLocation());
	}
}
```

---

### 6.3 Event Listener Binding (`GSItem.cpp`)

```cpp
void AGSItem::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		if (ItemData)
		{
			// Instantiate localized attribute sets defined in Data Asset
			for (const TSubclassOf<UAttributeSet>& SetClass : ItemData->AttributeSets)
			{
				AbilitySystemComponent->InitStats(SetClass, nullptr);
			}

			// Assign default tags
			if (ItemData->DefaultTags.Num() > 0)
			{
				AbilitySystemComponent->AddLooseGameplayTags(ItemData->DefaultTags, 1, EGameplayTagReplicationState::TagOnly);
			}

			// Subscribe item reactively to state tag activations
			for (const TPair<FGameplayTag, FGSItemStateDetails>& StatePair : ItemData->ItemStatesMap)
			{
				const FGameplayTag& StateTag = StatePair.Key;
				const FGSItemStateDetails& Details = StatePair.Value;

				if (Details.MaxProgressAttribute.IsValid())
				{
					AbilitySystemComponent->SetNumericAttributeBase(Details.MaxProgressAttribute, Details.MaxProgressValue);
				}

				AbilitySystemComponent->RegisterGameplayTagEvent(StateTag, EGameplayTagEventType::NewOrRemoved)
					.AddUObject(this, &AGSItem::OnStateTagChanged);
			}
		}
	}
}
```

---

## 7. Interactive Utility Stations (`AGSUtilityStation`)

### 7.1 Station Processing Engine (`UpdateEffectsForItem`)

```cpp
void AGSUtilityStation::UpdateEffectsForItem(AActor* Item)
{
	if (!HasAuthority() || !IsValid(Item)) { return; }

	UAbilitySystemComponent* TargetASC = nullptr;
	if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Item))
	{
		TargetASC = ASCInterface->GetAbilitySystemComponent();
	}
	if (!TargetASC) { return; }

	// 1. Clear previous station Gameplay Effects
	TArray<FActiveGameplayEffectHandle>* ActiveHandles = AppliedEffectsMap.Find(Item);
	if (ActiveHandles)
	{
		for (const FActiveGameplayEffectHandle& Handle : *ActiveHandles)
		{
			if (Handle.IsValid())
			{
				TargetASC->RemoveActiveGameplayEffect(Handle);
			}
		}
		ActiveHandles->Empty();
	}
	else
	{
		ActiveHandles = &AppliedEffectsMap.Add(Item);
	}

	// 2. Evaluate state-conditional effects (e.g., if item is Cooked, apply Burning effect)
	bool bAppliedConditional = false;
	for (const FGSConditionalEffectEntry& Entry : ConditionalEffectsFromData)
	{
		if (Entry.StateTag.IsValid() && TargetASC->HasMatchingGameplayTag(Entry.StateTag))
		{
			for (const TSubclassOf<UGameplayEffect>& EffectClass : Entry.EffectsToApply)
			{
				if (EffectClass)
				{
					FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
					Context.AddInstigator(this, this);
					FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.0f, Context);
					if (Spec.IsValid())
					{
						FActiveGameplayEffectHandle Handle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
						if (Handle.IsValid())
						{
							ActiveHandles->Add(Handle);
							bAppliedConditional = true;
						}
					}
				}
			}
		}
	}

	// 3. Fallback: Apply default base station effects if no conditional tags match
	if (!bAppliedConditional)
	{
		for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectsToApply)
		{
			if (EffectClass)
			{
				FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
				Context.AddInstigator(this, this);
				FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.0f, Context);
				if (Spec.IsValid())
				{
					FActiveGameplayEffectHandle Handle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
					if (Handle.IsValid())
					{
						ActiveHandles->Add(Handle);
					}
				}
			}
		}
	}
}
```

---

## 8. Networked Emotes & 3D Spatialized Audio

### 8.1 Multicast RPC Sound Spawning

```cpp
void AGSPawn::MulticastPlayEmoteSound_Implementation(UGSEmoteDefinition* EmoteDef)
{
	if (!EmoteDef) return;

	MulticastStopEmoteSound();

	USoundBase* SoundToPlay = EmoteDef->EmoteSound.LoadSynchronous();
	if (!SoundToPlay || GetNetMode() == NM_DedicatedServer) return;

	if (EmoteDef->bPlayAs3DSound)
	{
		ActiveEmoteAudioComponent = UGameplayStatics::SpawnSoundAttached(
			SoundToPlay, GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset, true, 1.f, 1.f, 0.f, nullptr, nullptr, true
		);

		if (ActiveEmoteAudioComponent)
		{
			ActiveEmoteAudioComponent->bAllowSpatialization = true;
			ActiveEmoteAudioComponent->AttenuationOverrides.bSpatialize = true;
			ActiveEmoteAudioComponent->AttenuationOverrides.AttenuationShape = EAttenuationShape::Sphere;
			ActiveEmoteAudioComponent->AttenuationOverrides.AttenuationShapeExtents = FVector(EmoteDef->SoundRadius * 0.1f, 0.f, 0.f);
			ActiveEmoteAudioComponent->AttenuationOverrides.FalloffDistance = EmoteDef->SoundRadius * 0.9f;
			ActiveEmoteAudioComponent->AdjustAttenuation(ActiveEmoteAudioComponent->AttenuationOverrides);
		}
	}
	else
	{
		ActiveEmoteAudioComponent = UGameplayStatics::SpawnSound2D(GetWorld(), SoundToPlay);
	}
}
```

---

## 9. NPC Customer Logic & Delivery Validation

### 9.1 Order Match & Cooked/Burned Validation

```cpp
bool UGSNPCComponent::CheckIfItemMatchesOrder(AGSItem* Item) const
{
	if (!Item || !bHasActiveOrder) return false;

	UAbilitySystemComponent* ItemASC = Item->GetAbilitySystemComponent();
	if (ItemASC)
	{
		bool bMatchesFood = ItemASC->HasMatchingGameplayTag(ActiveOrder.FoodTag);
		
		if (bRequireCookedState)
		{
			bool bIsCooked = ItemASC->HasMatchingGameplayTag(GSGameplayTags::State_Cooked);
			bool bIsBurned = ItemASC->HasMatchingGameplayTag(GSGameplayTags::State_Burned);

			// Logical exclusion: must be cooked and NOT burned
			return bMatchesFood && bIsCooked && !bIsBurned;
		}
		return bMatchesFood;
	}
	return Item->ItemTags.HasTagExact(ActiveOrder.FoodTag);
}
```

---

### 9.2 Currency Spawning on Eating Completion

```cpp
void UGSNPCComponent::HandleEatingFinished()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (MoneyItemClass && PendingMoneyValue > 0.0f)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		FVector SpawnLoc = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 20.0f);
		FRotator SpawnRot = GetOwner()->GetActorRotation();

		// Spawn physical currency actor on table for players to collect
		if (AGSMoneyItem* SpawnedMoney = GetWorld()->SpawnActor<AGSMoneyItem>(MoneyItemClass, SpawnLoc, SpawnRot, SpawnParams))
		{
			SpawnedMoney->MoneyValue = PendingMoneyValue;
		}
	}
	PendingMoneyValue = 0.0f;
	SetNPCState(ENPCState::Leaving);
}
```

---

## 10. StateTree C++ Task Implementations

### 10.1 Table Assignment Task (`GSStateTreeTask_AssignTable`)

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

---

### 10.2 Order Selection Task (`GSStateTreeTask_ChooseRandomOrder`)

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

---

### 10.3 Send To Exit Task (`GSStateTreeTask_SendToExit`)

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

---

## 11. Verse & Unreal Engine 6.0 Paradigm Shift

Unreal Engine 6.0 introduces **Verse** as a core native programming language. The architectural choices in this C++ framework were designed specifically to streamline future migration to Verse paradigms.

### 11.1 Reactive Input Streams vs. Polling Loops

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

---

### 11.2 Software Transactional Memory (STM) Reconciliation

Network prediction rollbacks in C++ require explicit comparison functions (`ShouldReconcile`). In Verse, prediction leverages **Software Transactional Memory (STM)**.

Transactions execute speculatively. If network desynchronization occurs, the runtime aborts the transaction block and reverts memory state atomically without custom C++ rewind routines.

---

### 11.3 Concurrent Async Coroutines for Mechanics

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

## 12. Extending the Framework

### 12.1 Writing Custom Movement Modes in C++

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

### 12.2 Writing Custom Attribute Sets & GAS Abilities

1. Inherit from `UAttributeSet` and define attributes using macro helpers (`ATTRIBUTE_ACCESSORS`).
2. Add attribute set classes to item or pawn data assets.
3. Inherit from `UGameplayAbility` to create active abilities reacting to tags.
