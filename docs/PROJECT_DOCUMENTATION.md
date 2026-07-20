# ProjectF (Good Service): Creator & Editor Workflow Guide

Welcome to the Creator & Editor Workflow Guide for **ProjectF (Good Service)** in Unreal Engine 5.8.

This document is designed with a fluid, welcoming, step-by-step approach. Whether you are a designer, level builder, or technical artist, this guide explains the core vision of the project, **why** systems were designed this way, and **how** to create game content in the Unreal Editor without touching C++ code.

---

## Creator's Vision: The Freedom to Imagine

> [!NOTE]
> **A Note on Design Philosophy & Creative Velocity**
> 
> Game creation is fundamentally about letting imagination flow without technical roadblocks. Traditional game engine setups force designers to wait for C++ recompilations or navigate tangled Blueprint inheritance trees just to add a new dish, a new hazard, or a new tool to the world.
> 
> **ProjectF** was built to eliminate that friction. By combining a single, generic `AGSItem` actor with data assets (`UGSItemDataAsset`), state tags, and station processing rules, **any** object can be created in seconds inside the Unreal Editor.
> 
> I studied the hard engineering behind Mover, GAS, StateTree, and Verse specifically to make game creation effortless for creators—so that your focus stays entirely on designing fun, creative gameplay.

---

## Table of Contents

1. [Design Philosophy & Project Vision](#1-design-philosophy--project-vision)
   - [1.1 Why Data-Driven Architecture?](#11-why-data-driven-architecture)
   - [1.2 Unlocking Creative Freedom](#12-unlocking-creative-freedom)
2. [Creator Guide: Creating Dynamic Items](#2-creator-guide-creating-dynamic-items)
   - [2.1 What is an Item?](#21-what-is-an-item)
   - [2.2 Step-by-Step: Creating a New Item Data Asset](#22-step-by-step-creating-a-new-item-data-asset)
   - [2.3 Mapping State Tags to Visual Actions](#23-mapping-state-tags-to-visual-actions)
   - [2.4 Spawning Items in Levels](#24-spawning-items-in-levels)
3. [Creator Guide: Interactive Utility Stations](#3-creator-guide-interactive-utility-stations)
   - [3.1 How Utility Stations Work](#31-how-utility-stations-work)
   - [3.2 Step-by-Step: Setting Up a Cooking Station](#32-step-by-step-setting-up-a-cooking-station)
   - [3.3 Socket Attachment & Item Placement](#33-socket-attachment--item-placement)
4. [Creator Guide: Customer NPCs, Menus & Rewards](#4-creator-guide-customer-npcs-menus--rewards)
   - [4.1 Setting Up Level Menus](#41-setting-up-level-menus)
   - [4.2 Customer AI StateTree Workflow](#42-customer-ai-statetree-workflow)
   - [4.3 Configuring Payment & Money Drops](#43-configuring-payment--money-drops)

---

## 1. Design Philosophy & Project Vision

### 1.1 Why Data-Driven Architecture?

In traditional game development, creating a new item—like a raw burger, a cooked pizza, or a money bundle—often requires writing a new C++ class or Blueprint subclass for every single variation. Over time, projects accumulate hundreds of repetitive blueprint files, creating maintenance debt and slowing down iteration.

**ProjectF** eliminates this bottleneck by using a **Data-Driven Architecture**. 
* Every interactive physical object in the world is represented by a single, generic C++ actor class: `AGSItem`.
* The behavior, mesh, attributes, sounds, and particle effects of an item are defined entirely by an external **Data Asset** (`UGSItemDataAsset`).

> [!TIP]
> **What does Data-Driven mean for Creators?**
> As a designer, you can add 50 new ingredients or tools to your game without writing a single line of C++ code or compiling Blueprints. You simply create Data Assets in the Content Browser and configure them in the Inspector.

---

### 1.2 Unlocking Creative Freedom

Because `AGSItem` is a generic entity carrying its own **Gameplay Ability System (GAS)** component, an item is not restricted to being a food ingredient.

With the same asset pipeline, creators can build:
* **Culinary Dishes:** Ingredients that cook on stoves, cool in refrigerators, or burn if neglected.
* **Hazard Objects:** Traps or radioactive containers that apply health reduction effects when touched.
* **Interactive Tools:** Keys, battery packs, or fuel cans that activate world machinery.
* **Currency Bundles:** Collectible cash stacks with custom monetary values.

Creators can iterate on gameplay mechanics in seconds directly inside the Unreal Editor without waiting for C++ code recompilation.

---

## 2. Creator Guide: Creating Dynamic Items

### 2.1 What is an Item?

An `AGSItem` is a dynamic actor that combines three core elements:
1. **Ability System Component (ASC):** Localized GAS component tracking attributes like cooking progress, temperature, or fill levels.
2. **Gameplay Tags:** Identifiers defining the item's identity (e.g., `Food.Burger`) and logical state (e.g., `State.Cooked`).
3. **Item Data Asset (`UGSItemDataAsset`):** Configuration blueprint defining meshes, sounds, attributes, and state transition actions.

---

### 2.2 Step-by-Step: Creating a New Item Data Asset

Follow these steps in the Unreal Editor to create a new item from scratch:

#### Step 1: Create the Data Asset
1. Open the **Content Browser**.
2. Navigate to your desired folder (e.g., `Content/DataAssets/Items/`).
3. Right-click in the empty space, select **Miscellaneous** > **Data Asset**.
4. Search for `GSItemDataAsset` in the class picker, select it, and click **Select**.
5. Name your asset (e.g., `DA_Steak`).

#### Step 2: Configure Default Gameplay Tags
1. Double-click your new `DA_Steak` asset to open the Inspector.
2. Under **Item Configuration** > **Default Tags**, click the plus icon to add tags:
   - Add identity tag: `Food.Steak`.
   - Add initial state tag: `State.Raw`.

#### Step 3: Assign Attribute Sets
1. Under **Item Configuration** > **Attribute Sets**, add the required attribute classes:
   - Add `GSCookingAttributeSet` (tracks cooking progress).
   - Add `GSBurnAttributeSet` (tracks burning progress).

---

### 2.3 Mapping State Tags to Visual Actions

Items automatically react to state changes. When a station applies heat and grants the tag `State.Cooked`, the item can automatically swap its 3D mesh, play a sizzling sound, or spawn steam particles.

```
+-------------------------------------------------------+
|                UGSItemDataAsset                       |
+-------------------------------------------------------+
|  ItemStatesMap:                                       |
|  - Key: State.Cooked                                  |
|  - Details:                                           |
|      * MaxProgressAttribute: CookingSet.MaxCookProgress |
|      * MaxProgressValue: 100.0                        |
|      * Actions:                                       |
|          [1] UGSItemStateAction_MeshOverride          |
|              -> Mesh: SM_Steak_Cooked                 |
|          [2] UGSItemStateAction_PlaySound             |
|              -> Sound: SFX_Sizzle                     |
+-------------------------------------------------------+
```

#### Under the Hood: C++ Action Polymorphism

Understanding **why** item actions work helps when extending the system. In C++, actions inherit from abstract base `UGSItemStateAction`:

```cpp
// Abstract base class for polymorphic item state actions
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTF_API UGSItemStateAction : public UObject
{
	GENERATED_BODY()
public:
	virtual void Execute(AActor* Owner) {}
};

// Action to override item static mesh
UCLASS(BlueprintType, EditInlineNew, meta=(DisplayName="Mesh Override"))
class PROJECTF_API UGSItemStateAction_MeshOverride : public UGSItemStateAction
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> MeshOverride = nullptr;

	virtual void Execute(AActor* Owner) override
	{
		if (!Owner || !MeshOverride) return;
		if (UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Owner->GetComponentByClass(UStaticMeshComponent::StaticClass())))
		{
			// Swap static mesh component instantly
			MeshComp->SetStaticMesh(MeshOverride);
		}
	}
};
```

> [!NOTE]
> **Why `EditInlineNew` and `DefaultToInstanced`?**
> In C++, marking action classes with `EditInlineNew` allows designers to instantiate action objects directly inside array properties in the Details Panel, without creating separate blueprint files for every action.

#### Step-by-Step Action Setup in Editor:
1. Inside your `DA_Steak` Data Asset, locate **Item States Map**.
2. Add a new key for tag `State.Cooked`.
3. Under **Actions**, click **+** and select **GS Item State Action Mesh Override**.
4. Pick your cooked steak static mesh asset (`SM_Steak_Cooked`).
5. Add another action for **GS Item State Action Play Sound** and assign your sound effect.
6. Save the Data Asset.

---

### 2.4 Spawning Items in Levels

To place your item in a level during design:
1. Drag the base `BP_GSItem` actor into the level viewport.
2. In the Details Panel, locate **Item Configuration** > **Item Data**.
3. Assign your newly created `DA_Steak` asset to the field.
4. The actor will automatically populate its mesh, attributes, and tags upon level launch.

---

## 3. Creator Guide: Interactive Utility Stations

### 3.1 How Utility Stations Work

`AGSUtilityStation` actors are world processing stations (stoves, fryers, sinks, cutting boards). 

When an item enters the station's collision volume (`StationVolume`), the station inspects the item's tags. If valid, it attaches the item to a 3D socket and applies continuous **Gameplay Effects (GE)** to alter the item's internal attributes.

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

	// 2. Apply state-conditional effects (e.g., if item is Cooked, apply Burning effect)
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

#### Code Breakdown: Why switch effects conditionally?

* **Raw to Cooked transition:** When an un-cooked burger sits on a stove, it receives `GE_CookingHeat`. Once cooking progress reaches 100%, the item gains `State.Cooked`.
* **Cooked to Burned transition:** `UpdateEffectsForItem` detects `State.Cooked`, cancels `GE_CookingHeat`, and seamlessly applies `GE_BurningHeat`.

---

### 3.2 Step-by-Step: Setting Up a Cooking Station

To set up a cooking stove in your level:

1. Drag `BP_GSUtilityStation_Stove` into the level viewport.
2. Select the station actor and inspect the **Station Configuration** category in the Details Panel.
3. Configure **Allowed Item Tags**:
   - Add `Food` (ensures only food items can be placed on the stove).
4. Configure **Effects To Apply**:
   - Add `GE_CookingHeat` (applies continuous positive delta to cooking progress attribute).
5. Configure **Conditional Effects**:
   - Add an entry with **State Tag** set to `State.Cooked`.
   - Set **Effects To Apply** to `GE_BurningHeat` (switches from cooking heat to burning heat once cooked).

---

### 3.3 Socket Attachment & Item Placement

Utility stations support automatic 3D snapping via static mesh sockets:
1. Open your station's static mesh asset in the Static Mesh Editor.
2. Create sockets named `ItemSocket_01`, `ItemSocket_02`, etc.
3. In the station details panel, enable **Use Station Sockets**.
4. When players drop items onto the station, items will automatically snap physically to available sockets.

---

## 4. Creator Guide: Customer NPCs, Menus & Rewards

### 4.1 Setting Up Level Menus

Restaurant levels utilize a `GSMenuDataAsset` to define available customer orders.

1. Create a new Data Asset of class `GSMenuDataAsset` (e.g., `DA_DinnerMenu`).
2. Add recipe entries to **Available Orders**:
   - Set **Food Tag** to `Food.Burger`.
   - Set **Display Name** to "Classic Burger".
   - Set **Base Price** to `15.0`.
3. Assign `DA_DinnerMenu` to the `AGSGameMode` or `AGSNPCManager` actor in your level.

---

### 4.2 Customer AI StateTree Workflow

Customer NPCs use Unreal Engine's **StateTree** framework for decision-making.

```
[Spawn at Door] 
       |
       v
[Assign Table State]  ---> Runs FGSStateTreeTask_AssignTable
       |
       v
[Walk to Table State] ---> Navigates via NavMoverComponent
       |
       v
[Order Meal State]    ---> Runs FGSStateTreeTask_ChooseRandomOrder
       |
       v
[Wait & Eat State]    ---> Listens for item delivery tag match
       |
       v
[Exit State]          ---> Runs FGSStateTreeTask_SendToExit & Spawns Money
```

Creators can adjust customer patience settings inside `UGSNPCComponent` in the Details Panel:
* **Max Patience Seconds:** Time customer will wait for food before leaving enraged.
* **Require Cooked State:** If checked, delivery validation requires `State.Cooked` and excludes `State.Burned`.

---

### 4.3 Configuring Payment & Money Drops

When a customer finishes eating successfully:

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

		// Spawn physical currency actor onto table
		if (AGSMoneyItem* SpawnedMoney = GetWorld()->SpawnActor<AGSMoneyItem>(MoneyItemClass, SpawnLoc, SpawnRot, SpawnParams))
		{
			SpawnedMoney->MoneyValue = PendingMoneyValue;
		}
	}
	PendingMoneyValue = 0.0f;
	SetNPCState(ENPCState::Leaving);
}
```

1. The NPC component retrieves the recipe base price.
2. It automatically spawns a physical `AGSMoneyItem` actor onto the table.
3. The player can walk over or grab the money item to add funds to the team bank account via `GSMoneyComponent`.
