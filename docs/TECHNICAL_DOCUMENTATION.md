# Technical Documentation: System Implementation and Extension Manual

This document serves as a technical reference manual and implementation guide for developers. It details step-by-step software design for project systems, providing full C++ code snippets and explaining how to extend existing systems to create new items, stations, emotes, manipulation abilities, and NPC behaviors.

---

## 1. How to Implement New Objects / Items

All dynamic game objects (raw ingredients, cooked dishes, money items, etc.) are represented by the `AGSItem` class. Rather than relying on rigid C++ class hierarchies, this architecture uses a data-driven approach by parameterizing a single generic actor through a **Data Asset** (`UGSItemDataAsset`).

### Gameplay Ability System (GAS) Architecture in Items
A key software design pillar is equipping each `AGSItem` actor with its own `UAbilitySystemComponent` (ASC):
* **First-Class GAS Entities:** Items can directly receive Gameplay Effects (GE) from world stations (e.g., heat from an oven or cold from a freezer).
* **Self-Contained Local Attributes:** Cooking, cooling, and filling are tracked via local item attribute sets (`AttributeSets`), avoiding pollution of character classes.
* **Dynamic State Mapping:** Visual variations and logical state changes are managed by linking Gameplay Tags to visual behaviors defined in the item's Data Asset.

```
+-------------------------------------------------------------+
|                          AGSItem                            |
+-------------------------------------------------------------+
| - AbilitySystemComponent                                    |
| - AttributeSets (Array of local sets)                       |
| - ItemData (Ref to UGSItemDataAsset)                        |
+------------------------------+------------------------------+
                               |
                               v  Loads configuration from
+------------------------------+------------------------------+
|                    UGSItemDataAsset                         |
+-------------------------------------------------------------+
| - DefaultTags (Initial item tags)                           |
| - AttributeSets (Required attribute set classes)            |
| - ItemStatesMap (State tag to details map)                  |
+------------------------------+------------------------------+
                               |
                               v  Defines behavior in
+------------------------------+------------------------------+
|                   FGSItemStateDetails                       |
+-------------------------------------------------------------+
| - MaxProgressAttribute (Limit attribute)                    |
| - Actions (Array of UGSItemStateAction)                     |
|     * UGSItemStateAction_MeshOverride                       |
|     * UGSItemStateAction_PlaySound                          |
|     * UGSItemStateAction_SpawnParticles                     |
+-------------------------------------------------------------+
```

> [!NOTE]
> **Verse Perspective in Unreal Engine 6.0: Data-Driven Native Objects**
> In future **Verse programming in Unreal Engine 6.0**, traditional Data Assets like `UGSItemDataAsset` and rigid C++ structs will be replaced by native data types (`struct` and `class` with immutable fields). Visual state transition behaviors will be bound directly to asynchronous event channels.

### Item Data Asset Definition (`GSItemDataAsset.h`)
The Data Asset defines state maps and associated polymorphic actions (static mesh overrides, sound effects, particle spawners):

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AttributeSet.h"
#include "GSItemDataAsset.generated.h"

// Abstract base class for actions associated with item state transitions
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

// Action to play sound effect
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

### Action Execution Logic (`GSItemDataAsset.cpp`)
Polymorphic actions execute local visual and auditory state changes cleanly:
```cpp
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

### Event Binding in Items (`GSItem.cpp`)
In `AGSItem::BeginPlay`, we register listeners to tag delegates on the item's ASC, triggering actions when state tags are added or removed:
```cpp
void AGSItem::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// Instantiate required attribute sets defined in Data Asset
		if (ItemData)
		{
			for (const TSubclassOf<UAttributeSet>& SetClass : ItemData->AttributeSets)
			{
				AbilitySystemComponent->InitStats(SetClass, nullptr);
			}

			// Add initial tags and register state callbacks
			if (ItemData->DefaultTags.Num() > 0)
			{
				AbilitySystemComponent->AddLooseGameplayTags(ItemData->DefaultTags, 1, EGameplayTagReplicationState::TagOnly);
			}

			for (const TPair<FGameplayTag, FGSItemStateDetails>& StatePair : ItemData->ItemStatesMap)
			{
				const FGameplayTag& StateTag = StatePair.Key;
				const FGSItemStateDetails& Details = StatePair.Value;

				if (Details.MaxProgressAttribute.IsValid())
				{
					AbilitySystemComponent->SetNumericAttributeBase(Details.MaxProgressAttribute, Details.MaxProgressValue);
				}

				// Subscribe item to state tag activation
				AbilitySystemComponent->RegisterGameplayTagEvent(StateTag, EGameplayTagEventType::NewOrRemoved)
					.AddUObject(this, &AGSItem::OnStateTagChanged);
			}
		}
	}
}
```

---

## 2. How to Create Interactive Stations (`AGSUtilityStation`)

Utility stations (`AGSUtilityStation`) are interactive static world actors responsible for applying processing logic (cooking, cooling, draining) to items.

### Collision and Item Detection
The station collision volume (`StationVolume`) ignores pawns and reacts only to actors implementing `IAbilitySystemInterface` with tags allowed in `AllowedItemTags`.
* **Item Placement:** Upon collision overlap, if an item is not held by a pawn, the station adds it to `PlacedItems` and calls `OnItemAddedToStation`.
* **Socket Attachment:** If free attachment sockets (`StationSockets`) exist, the item physically attaches to the 3D model in the first available socket via `GetFirstFreeSocket()`.
* **Item Grab:** When a player grabs (`Grab`) an item on the station, callback `OnPlacedItemGrabbed` removes the item from the station's placement array, clears active effects, and restores physics control to the pawn.

### Base and Conditional Effect Application (`UpdateEffectsForItem`)
This function manages server-side dynamic processing logic. It applies station base effects (`EffectsToApply`) or switches to state-conditional effects (`ConditionalEffects`) if an item reaches a specific state (e.g., dish is cooked and begins burning).

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

	// 2. Evaluate and apply conditional effects based on item active tags
	bool bAppliedConditional = false;
	for (const FGSConditionalEffectEntry& Entry : ConditionalEffectsFromData)
	{
		if (Entry.StateTag.IsValid() && TargetASC->HasMatchingGameplayTag(Entry.StateTag))
		{
			for (const TSubclassOf<UGameplayEffect>& EffectClass : Entry.EffectsToApply)
			{
				if (EffectClass)
				{
					FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
					EffectContext.AddInstigator(this, this);
					FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
					if (SpecHandle.IsValid())
					{
						FActiveGameplayEffectHandle Handle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
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

	// 3. If no conditional tags active, apply default base station effects
	if (!bAppliedConditional)
	{
		for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectsToApply)
		{
			if (EffectClass)
			{
				FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
				EffectContext.AddInstigator(this, this);
				FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
				if (SpecHandle.IsValid())
				{
					FActiveGameplayEffectHandle Handle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
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

## 3. Ability System (GAS), Emotes, and Grab/Launch Mechanics

### Emote System and Multicast RPCs
The emote system plays networked 3D spatialized audio via `GSAbility_PlayEmote` and pawn RPCs:
1. Server receives call and invokes Multicast RPC `MulticastPlayEmoteSound_Implementation`.
2. Clients load audio synchronously and spawn a 3D sound component.
3. 3D spatialization parameters (attenuation and falloff distance) are configured based on sound radius specified in `UGSEmoteDefinition`.

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

### Vertical Grab and Stacking (`GSAbility_Grab`)
Grab ability (`UGSAbility_Grab`) calculates relative placement positions overhead:
* If it is the first item, it aligns with front socket position.
* If pawn already carries items, ability recursively calculates cumulative stack height:

$$Offset_Z = BaseOffset + InitialOffset + \sum_{i=1}^{StackCount} Height_i + (StackCount \times SpacingOffset)$$

```cpp
float CumulativeHeight = 0.0f;
int32 StackCount = 0;
TArray<AActor*> CurrAttached;
AvatarActor->GetAttachedActors(CurrAttached);
for (AActor* AttachedActor : CurrAttached)
{
	if (AttachedActor && AttachedActor != BestTarget)
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

### Kinematic Launching vs. Chaos Physics
Unreal Engine's **Chaos** physics engine runs asynchronously via sub-stepping. This prevents Mover's Network Prediction system from executing deterministic local client rollbacks during network desyncs if throwing motion is delegated purely to dynamic Chaos physics.

Therefore, we implement an explicit kinematic trajectory simulation (`LaunchKinematic`) in the launch ability (`UGSAbility_Launch`). This computes step-by-step parabolic motion and interpolates position directly in `UGSGrabbableComponent::TickComponent`, avoiding physics jitter in multiplayer:

```cpp
float ScaledSpeed = FMath::Lerp(GrabComp->ThrowSpeed * 0.4f, GrabComp->ThrowSpeed, ClampedDistance / MaxThrowDistance);
float EstimatedPathLength = HorizontalDistance + (1.5f * LaunchZ);
float ThrowDuration = EstimatedPathLength / ScaledSpeed;
GrabComp->LaunchKinematic(LaunchStartLoc, DispersedTarget, LaunchZ, ThrowDuration);
```

---

## 4. NPC Configuration and Lifecycle

Restaurant NPC customers operate via local state machine `ENPCState` inside `UGSNPCComponent`.

### Order Lifecycle and Delivery Validation
1. **Recipe Selection (`ChooseRandomOrder`):** Upon seating, StateTree instructs recipe selection. NPC gets menu Data Asset (`LevelMenu`), picks a item, and triggers order event (`OnOrderChosen`).
2. **Delivery Validation (`CheckIfItemMatchesOrder`):** When player delivers an item, component checks Gameplay Tags on item ASC:
   * Must match target food tag (e.g., `Food.Burger`).
   * If `bRequireCookedState` is true, requires tag `State.Cooked` and excludes `State.Burned`.

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

			return bMatchesFood && bIsCooked && !bIsBurned;
		}
		return bMatchesFood;
	}
	return Item->ItemTags.HasTagExact(ActiveOrder.FoodTag);
}
```

3. **Payment and Tip Processing (`DeliverItem` & `HandleEatingFinished`):**
   * Upon successful match, stores base price in `PendingMoneyValue`, destroys item, and starts eating timer (`EatingTimerHandle`).
   * When eating timer finishes, NPC spawns physical `AGSMoneyItem` actor at table with assigned monetary value before departing to exit.

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

		if (AGSMoneyItem* SpawnedMoney = GetWorld()->SpawnActor<AGSMoneyItem>(MoneyItemClass, SpawnLoc, SpawnRot, SpawnParams))
		{
			SpawnedMoney->MoneyValue = PendingMoneyValue;
		}
	}
	PendingMoneyValue = 0.0f;
	SetNPCState(ENPCState::Leaving);
}
```
