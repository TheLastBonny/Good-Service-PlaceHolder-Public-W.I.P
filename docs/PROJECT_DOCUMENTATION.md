# ProjectF (Good Service): Creator & Editor Workflow Guide

Welcome to the comprehensive Creator & Editor Workflow Guide for **ProjectF (Good Service)** in Unreal Engine 5.8.

This document serves as the primary manual for level designers, content creators, and technical artists. It strikes a balance between **accessibility** and **depth**—explaining the architectural vision, the mechanics of the data-driven framework, and step-by-step instructions for creating game content inside the Unreal Editor without C++ code recompilations.

---

## Table of Contents

1. [Creator's Vision & Design Philosophy](#1-creators-vision--design-philosophy)
2. [Quick Start Overview](#2-quick-start-overview)
3. [Data-Driven Fundamentals](#3-data-driven-fundamentals)
   - [3.1 The Trinity: Tags + Data Assets + Generic Actor](#31-the-trinity-tags--data-assets--generic-actor)
   - [3.2 Traditional OOP vs. ProjectF Comparison](#32-traditional-oop-vs-projectf-comparison)
4. [Creator Guide: Creating & Customizing Items](#4-creator-guide-creating--customizing-items)
   - [4.1 Anotomy of an Item](#41-anotomy-of-an-item)
   - [4.2 Understanding States, Progress Attributes & Actions](#42-understanding-states-progress-attributes--actions)
   - [4.3 Step-by-Step: Creating a Multi-State Item](#43-step-by-step-creating-a-multi-state-item)
   - [4.4 Creative Possibilities: Beyond Food Ingredients](#44-creative-possibilities-beyond-food-ingredients)
5. [Creator Guide: Interactive Utility Stations](#5-creator-guide-interactive-utility-stations)
   - [5.1 How Stations Process Items](#51-how-stations-process-items)
   - [5.2 Base Effects vs. Conditional Effects](#52-base-effects-vs-conditional-effects)
   - [5.3 Socket Attachment & Snapping](#53-socket-attachment--snapping)
   - [5.4 Step-by-Step: Setting Up a Cooking Station](#54-step-by-step-setting-up-a-cooking-station)
6. [Creator Guide: Customer NPCs, Navigation & Menus](#6-creator-guide-customer-npcs-navigation--menus)
   - [6.1 NPC Locomotion: Mover + NavMover Component](#61-npc-locomotion-mover--navmover-component)
   - [6.2 StateTree Decision Flow](#62-statetree-decision-flow)
   - [6.3 Restaurant Menus, Order Delivery & Currency Drops](#63-restaurant-menus-order-delivery--currency-drops)
7. [Best Practices & Advanced Designer Tips](#7-best-practices--advanced-designer-tips)
8. [Next Steps & References](#8-next-steps--references)

---

## 1. Creator's Vision & Design Philosophy

> [!NOTE]
> **A Note on Creative Velocity & Frictionless Development**
> 
> Game creation is fundamentally about letting imagination flow without technical roadblocks. Traditional game engine setups force designers to wait for C++ recompilations or navigate tangled Blueprint inheritance trees just to add a new dish, a new hazard, or a new tool to the world.
> 
> **ProjectF** was built to eliminate that friction. By combining a single, generic `AGSItem` actor with data assets (`UGSItemDataAsset`), state tags, and station processing rules, **any** object can be created in seconds inside the Unreal Editor.
> 
> I studied the complex engineering behind Mover, GAS, StateTree, and Verse specifically to make game creation effortless for creators—so that your focus stays entirely on designing fun, creative gameplay.

---

## 2. Quick Start Overview

If you are looking for an immediate 5-minute hands-on tutorial to build your first item and station without reading background theory, jump directly to our **[5-Minute Creator Quick Start Guide](QUICK_START.md)**.

For a deep understanding of how to build complex multi-state systems, level menus, and interactive stations, continue reading below.

---

## 3. Data-Driven Fundamentals

### 3.1 The Trinity: Tags + Data Assets + Generic Actor

Traditional game development relies on **Object-Oriented Programming (OOP)**, where every object variation requires its own Blueprint or C++ subclass. In a restaurant or service game, this results in `BP_Burger_Raw`, `BP_Burger_Cooked`, `BP_Burger_Burned`, `BP_Pizza_Raw`, etc.

**ProjectF** replaces this bloated inheritance tree with **The Data-Driven Trinity**:

```
+-------------------------------------------------------+
|  1. Generic Actor (AGSItem)                           |
|     Physical container carrying collision & GAS       |
+---------------------------+---------------------------+
                            | Driven by
                            v
+-------------------------------------------------------+
|  2. Data Asset (UGSItemDataAsset)                     |
|     Defines meshes, attribute sets & state rules      |
+---------------------------+---------------------------+
                            | Categorized by
                            v
+-------------------------------------------------------+
|  3. Gameplay Tags (FGameplayTag)                      |
|     Identity (Food.Steak) & State (State.Cooked)      |
+-------------------------------------------------------+
```

1. **Generic Actor (`AGSItem`):** A single physical C++ actor placed in the world. It carries an Ability System Component (ASC) and reads configurations from its assigned Data Asset.
2. **Data Asset (`UGSItemDataAsset`):** A blueprint asset configured in the Editor defining the item's static mesh, audio, particles, progress limits, and state transition actions.
3. **Gameplay Tags (`FGameplayTag`):** Lightweight, hierarchical strings (e.g., `Food.Steak`, `State.Cooked`) that define what an item is and what state it currently occupies.

---

### 3.2 Traditional OOP vs. ProjectF Comparison

| Workflow Aspect | Traditional OOP Engine Approach | ProjectF Data-Driven Workflow |
| :--- | :--- | :--- |
| **Creating 50 New Items** | Create 50 separate Blueprint classes + inheritance | Create 50 Data Assets in the Content Browser |
| **Iterating on Visuals** | Open and modify 50 Blueprint graphs | Swap mesh properties in Data Asset Inspectors |
| **Adding a New Gameplay Effect** | Edit class C++ code or Blueprint event graphs | Add Gameplay Tag to station allowed list |
| **Memory Footprint** | Dozens of loaded class types in RAM | 1 generic actor class + lightweight data tables |
| **Compile Time Impact** | Triggers C++ or Blueprint compilation | Zero compilation impact (instant PIE testing) |

---

## 4. Creator Guide: Creating & Customizing Items

### 4.1 Anatomy of an Item

An item data asset (`UGSItemDataAsset`) contains three primary configuration categories in the Details Panel:

1. **Default Tags (`FGameplayTagContainer`):** Initial identity and state tags assigned when spawned (e.g., `Food.Burger`, `State.Raw`).
2. **Attribute Sets (`TArray<TSubclassOf<UAttributeSet>>`):** Attribute sets instantiated locally on the item's ASC (e.g., `GSCookingAttributeSet` for cooking progress, `GSBurnAttributeSet` for burning progress).
3. **Item States Map (`TMap<FGameplayTag, FGSItemStateDetails>`):** Maps specific state tags to progress limits and visual state actions.

---

### 4.2 Understanding States, Progress Attributes & Actions

Each state entry in the **Item States Map** defines how an item behaves when a specific Gameplay Tag becomes active:

```
+-------------------------------------------------------+
|  Item States Map Key: State.Cooked                    |
+-------------------------------------------------------+
|  MaxProgressAttribute : CookingSet.MaxCookProgress    |
|  MaxProgressValue     : 100.0                         |
|  StateName            : "Cooked Steak"                |
|  Actions:                                             |
|    - [Action 1] Mesh Override -> SM_Steak_Cooked      |
|    - [Action 2] Play Sound    -> SFX_Sizzle           |
+-------------------------------------------------------+
```

#### Key Concepts:

* **Max Progress Attribute:** Specifies which attribute limits this state (e.g., when `CookingSet.CookProgress` reaches `MaxProgressValue`, the item transitions to cooked).
* **Polymorphic Actions (`UGSItemStateAction`):** Self-contained action triggers executed automatically when a tag is added or removed. Designers can choose:
  * **Mesh Override Action:** Instantly swaps the item's static mesh asset.
  * **Play Sound Action:** Plays 3D spatialized audio at the item's location.
  * **Particle Action:** Spawns particle emitters (e.g., steam or smoke).

> [!TIP]
> **Action Extensibility**
> Actions are modular sub-objects. Designers can stack multiple actions under a single state—for example, swapping the mesh to burnt charcoal, spawning thick black smoke particles, and playing a burning sizzle sound simultaneously.

---

### 4.3 Step-by-Step: Creating a Multi-State Item

Let's walk through creating a complete item with 3 states: **Raw**, **Cooked**, and **Burned**.

#### Step 1: Create and Label the Data Asset
1. Right-click in Content Browser > **Miscellaneous** > **Data Asset**.
2. Pick `GSItemDataAsset` and name it `DA_Burger`.
3. In **Default Tags**, add `Food.Burger` and `State.Raw`.
4. In **Attribute Sets**, add `GSCookingAttributeSet` and `GSBurnAttributeSet`.

#### Step 2: Configure Raw State (Initial State)
1. In **Item States Map**, add key `State.Raw`.
2. Add a **Mesh Override Action** assigned to `SM_Burger_Raw`.

#### Step 3: Configure Cooked State
1. In **Item States Map**, add key `State.Cooked`.
2. Set **Max Progress Attribute** to `GSCookingAttributeSet.MaxCookProgress` and **Max Progress Value** to `100.0`.
3. Add a **Mesh Override Action** assigned to `SM_Burger_Cooked`.
4. Add a **Play Sound Action** assigned to `SFX_Sizzle`.

#### Step 4: Configure Burned State
1. In **Item States Map**, add key `State.Burned`.
2. Set **Max Progress Attribute** to `GSBurnAttributeSet.MaxBurnProgress` and **Max Progress Value** to `100.0`.
3. Add a **Mesh Override Action** assigned to `SM_Burger_Burned`.
4. Save the asset.

---

### 4.4 Creative Possibilities: Beyond Food Ingredients

Because `AGSItem` is a generic entity driven by tags and attribute sets, you are not limited to restaurant recipes:

* **Hazardous Barrels:** Assign a `HealthAttributeSet`. When placed near fire stations, the item takes damage and triggers an explosion particle action upon reaching zero health.
* **Keycards & Battery Packs:** Assign identity tag `Tool.Battery`. World doors check for `Tool.Battery` to grant access.
* **Physical Currency Stacks:** Assign identity tag `Item.Money` and bind monetary values to `GSMoneyValueComponent`.

---

## 5. Creator Guide: Interactive Utility Stations

### 5.1 How Stations Process Items

`AGSUtilityStation` actors are world processing stations (stoves, refrigerators, cutting boards, sinks). 

When an item enters the station's collision volume (`StationVolume`), the station inspects the item's tags:
1. **Tag Filtering:** Checks if the item possesses any tag listed in `AllowedItemTags`.
2. **Socket Snapping:** If free sockets exist (`StationSockets`), the item physically attaches to the station model.
3. **Effect Application:** The station applies continuous **Gameplay Effects (GE)** to alter the item's attribute sets.

---

### 5.2 Base Effects vs. Conditional Effects

Stations feature two levels of processing rules:

* **Base Effects (`EffectsToApply`):** Applied continuously to any valid item placed on the station (e.g., a stove always applies `GE_CookingHeat`).
* **Conditional Effects (`ConditionalEffectsFromData`):** Applied only when an item possesses a specific state tag. 

```
[Raw Item Enters Stove] ---> Stove Applies GE_CookingHeat
                                       |
                                       v (Reaches 100% Progress)
[Item Gains State.Cooked Tag] --------+
                                       |
                                       v (Triggers Conditional Rule)
[Stove Swaps to GE_BurningHeat] ------> Item Begins Burning Progress
```

---

### 5.3 Socket Attachment & Snapping

To ensure items rest neatly on countertops or stove burners:
1. Open your station's static mesh asset in the Unreal Static Mesh Editor.
2. Create sockets named `ItemSocket_01`, `ItemSocket_02`, `ItemSocket_03`.
3. In the station Details Panel, check **Use Station Sockets**.
4. When dropped, items will automatically snap to the nearest available socket position.

---

### 5.4 Step-by-Step: Setting Up a Cooking Station

1. Drag `BP_GSUtilityStation_Stove` into your level.
2. Under **Station Configuration** > **Allowed Item Tags**, add tag `Food`.
3. Under **Effects To Apply**, add `GE_CookingHeat`.
4. Under **Conditional Effects**, add a new entry:
   - Set **State Tag** to `State.Cooked`.
   - Set **Effects To Apply** to `GE_BurningHeat`.
5. Place a raw food item on the stove in Play-In-Editor to test!

---

## 6. Creator Guide: Customer NPCs, Navigation & Menus

### 6.1 NPC Locomotion: Mover + NavMover Component

NPC locomotion in **ProjectF** utilizes the **Mover Framework** coupled with `UNavMoverComponent`:

```
+-------------------------------------------------------+
|  AGSPawnNPC (Customer Pawn)                           |
+-------------------------------------------------------+
|  - UCharacterMoverComponent (Physics Locomotion)     |
|  - UNavMoverComponent       (Pathfinding Intent)     |
|  - UGSNPCComponent          (Order & Patience Logic)  |
+-------------------------------------------------------+
```

> [!IMPORTANT]
> **Smooth NPC Navigation via Mover**
> Pathfinding directives from Unreal's Navigation System are consumed by `UNavMoverComponent` and passed directly into Mover's simulation tick context (`FMoverInputCmdContext`). This guarantees that NPC customers move with the exact same high-performance, constraint-based physical locomotion solver as human player pawns.

---

### 6.2 StateTree Decision Flow

Customer NPCs navigate the restaurant using a flat, high-performance **StateTree** decision tree (`ST_NPC_Customer`):

```
[Spawn at Entrance]
        |
        v
[Assign Table State]  ---> Runs GS Assign Table Task
        |
        v
[Walk to Table State] ---> Navigates via NavMoverComponent
        |
        v
[Order Meal State]    ---> Runs GS Choose Random Order Task
        |
        v
[Wait & Eat State]    ---> Patience timer active; awaits food delivery
        |
        v
[Exit State]          ---> Runs GS Send To Exit Task & Spawns Money
```

---

### 6.3 Restaurant Menus, Order Delivery & Currency Drops

#### Level Menu Setup (`GSMenuDataAsset`)
1. Create a Data Asset of class `GSMenuDataAsset` (e.g., `DA_DinnerMenu`).
2. Under **Available Orders**, add recipes:
   - **Food Tag:** `Food.Burger`
   - **Display Name:** "Classic Burger"
   - **Base Price:** `15.0`
3. Assign `DA_DinnerMenu` to the `AGSNPCManager` actor in your level.

#### Delivery Validation & Payment
When a player delivers an item to a seated customer, `UGSNPCComponent` validates the delivery:
* **Tag Matching:** Checks if the item's ASC has matching `FoodTag` (e.g., `Food.Burger`).
* **Cooked State Validation:** If `bRequireCookedState` is enabled in customer settings, the item must possess `State.Cooked` and must not possess `State.Burned`.
* **Money Drop:** Upon completing eating, the customer automatically spawns an `AGSMoneyItem` physical cash drop containing the recipe base price on the table.

---

## 7. Best Practices & Advanced Designer Tips

1. **Tag Taxonomy:** Maintain clean Gameplay Tag hierarchies. Use broad categories (`Food`, `Tool`, `Hazard`) for station filtering and specific sub-tags (`Food.Burger`, `Food.Steak`) for customer orders.
2. **Reuse Action Objects:** Create generic sound and particle action assets that can be shared across multiple item data assets.
3. **Patience Balancing:** Adjust customer `MaxPatienceSeconds` inside `UGSNPCComponent` to tune difficulty for single-player versus co-op multiplayer sessions.

---

## 8. Next Steps & References

* **5-Minute Hands-On Tutorial:** **[5-Minute Creator Quick Start Guide](QUICK_START.md)**
* **Low-Level C++ & Verse Reference:** **[Mover & GAS Technical Framework Guide](TECHNICAL_DOCUMENTATION.md)**
* **Community Guidelines:** **[CONTRIBUTING.md](../CONTRIBUTING.md)**
