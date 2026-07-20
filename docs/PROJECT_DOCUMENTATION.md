# ProjectF (Good Service): Creator & Editor Workflow Guide

Welcome to the Creator & Editor Workflow Guide for **ProjectF (Good Service)** in Unreal Engine 5.8.

This document is designed with a fluid, welcoming, step-by-step approach. Whether you are a level designer, content creator, or technical artist, this guide explains the core vision of the project, **why** systems were designed this way, and **how** to build rich game mechanics in the Unreal Editor without touching C++ code.

---

## Creator's Vision: The Freedom to Imagine

> [!NOTE]
> **A Note on Design Philosophy & Creative Velocity**
> 
> Game creation is fundamentally about letting imagination flow without technical roadblocks. Traditional game engine setups force designers to wait for C++ recompilations or navigate tangled Blueprint inheritance trees just to add a new dish, a new hazard, or a new tool to the world.
> 
> **ProjectF** was built to eliminate that friction. By combining a single, generic `AGSItem` actor with data assets (`UGSItemDataAsset`), state tags, and station processing rules, **any** object can be created in seconds inside the Unreal Editor.
> 
> I studied the complex engineering behind Mover, GAS, StateTree, and Verse specifically to make game creation effortless for creators—so that your focus stays entirely on designing fun, creative gameplay.

---

## Table of Contents

1. [Design Philosophy & Project Vision](#1-design-philosophy--project-vision)
   - [1.1 Why Data-Driven Architecture?](#11-why-data-driven-architecture)
   - [1.2 Traditional OOP vs. ProjectF Comparison](#12-traditional-oop-vs-projectf-comparison)
   - [1.3 Unlocking Creative Freedom](#13-unlocking-creative-freedom)
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
* Every interactive physical object in the world is represented by a single, generic actor class: `AGSItem`.
* The behavior, mesh, attributes, sounds, and particle effects of an item are defined entirely by an external **Data Asset** (`UGSItemDataAsset`).

---

### 1.2 Traditional OOP vs. ProjectF Comparison

| Workflow Feature | Traditional OOP Approach | ProjectF (Data-Driven Workflow) |
| :--- | :--- | :--- |
| **Creating 10 new items** | Create 10 new Blueprint classes + inheritance | Create 10 Data Assets in the Content Browser |
| **Setup time per item** | 30 to 60 minutes of Blueprint scripting | 2 minutes in the Inspector panel |
| **Asset Maintenance** | 50+ duplicated Blueprint files to manage | 1 single generic actor + data files |
| **Recompiling Code** | Required after structural changes | Zero code recompilation required |

---

### 1.3 Unlocking Creative Freedom

Because `AGSItem` is a generic entity carrying its own **Gameplay Ability System (GAS)** component, an item is not restricted to being a food ingredient.

With the exact same asset pipeline, creators can build:
* **Culinary Dishes:** Ingredients that cook on stoves, cool in refrigerators, or burn if neglected.
* **Hazard Objects:** Traps or radioactive containers that apply health reduction effects when touched.
* **Interactive Tools:** Keys, battery packs, or fuel cans that activate world machinery.
* **Currency Bundles:** Collectible cash stacks with custom monetary values.

Creators can iterate on gameplay mechanics in seconds directly inside the Unreal Editor without waiting for C++ code recompilation.

---

## 2. Creator Guide: Creating Dynamic Items

### 2.1 What is an Item?

An `AGSItem` is a dynamic actor that combines three core elements:
1. **Ability System Component (ASC):** Localized component tracking attributes like cooking progress, temperature, or fill levels.
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
|          [1] GS Item State Action Mesh Override       |
|              -> Mesh: SM_Steak_Cooked                 |
|          [2] GS Item State Action Play Sound          |
|              -> Sound: SFX_Sizzle                     |
+-------------------------------------------------------+
```

> [!TIP]
> **Polymorphic Actions in Inspector**
> Designers can instantiate action objects directly inside array properties in the Details Panel, selecting mesh overrides, sound triggers, or particle spawners without writing code.

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

```
[Raw Item Enters Stove Volume]
              |
              v
[Stove Applies GE_CookingHeat] ---> Increases Cooking Progress Attribute
              |
              v (Reaches 100% Progress)
[Item Gains Tag: State.Cooked] ---> Triggers Mesh Override Action to SM_Steak_Cooked
              |
              v
[Stove Switches to GE_BurningHeat] -> Increases Burning Progress Attribute
```

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
[Assign Table State]  ---> Reserves Spot at Table
       |
       v
[Walk to Table State] ---> Navigates via NavMoverComponent
       |
       v
[Order Meal State]    ---> Chooses Order from Level Menu
       |
       v
[Wait & Eat State]    ---> Listens for item delivery tag match
       |
       v
[Exit State]          ---> Frees Table & Spawns Currency Drop
```

Creators can adjust customer patience settings inside `UGSNPCComponent` in the Details Panel:
* **Max Patience Seconds:** Time customer will wait for food before leaving enraged.
* **Require Cooked State:** If checked, delivery validation requires `State.Cooked` and excludes `State.Burned`.

---

### 4.3 Configuring Payment & Money Drops

When a customer finishes eating successfully:
1. The NPC component retrieves the recipe base price.
2. It automatically spawns a physical `AGSMoneyItem` actor onto the table.
3. The player can walk over or grab the money item to add funds to the team bank account via `GSMoneyComponent`.
