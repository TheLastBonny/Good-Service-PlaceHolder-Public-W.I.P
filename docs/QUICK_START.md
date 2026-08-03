# 5-Minute Creator Quick Start Guide

Welcome to the **5-Minute Creator Quick Start Guide** for **ProjectF (Good Service)** in Unreal Engine 5.8!

This guide is designed for level designers, content creators, and newcomers. In less than 5 minutes, you will learn how to create a brand new interactive item (a Croissant), set up visual cooking states, adjust physical grabbing interaction, and test it on a cooking station—**without writing a single line of C++ or Blueprint code**.

---

## Traditional Game Development vs. ProjectF

Before jumping in, consider how game creation changes with a Data-Driven Workflow:

| Workflow Action | Traditional Engine Approach (OOP) | ProjectF (Data-Driven Workflow) |
| :--- | :--- | :--- |
| **Creating 10 new items** | Create 10 new Blueprint classes + complex inheritance | Create 10 simple Data Assets in the Content Browser |
| **Setup Time per Item** | 30 to 60 minutes of Blueprint scripting | 2 minutes in the Inspector panel |
| **Asset Maintenance** | 50+ duplicated Blueprint files to manage | 1 single generic actor + data files |
| **Code Recompilation** | Required after C++ structural changes | Zero code recompilation required (instant PIE testing) |

---

## Step 1: Create your First Item Data Asset (2 Minutes)

Imagine every interactive object in your game—food, tools, traps, or money—sharing a single, highly optimized physical actor. You never need to create a new Blueprint class for a new item; you simply create a **Data Asset**.

1. Open the **Content Browser** in Unreal Editor.
2. Navigate to `Content/Datas/ItemsData/`.
3. **Right-click** in an empty space and select **Data** > **Data Asset**.
4. In the class picker search bar, type `GSItemDataAsset`, select it, and click **Select**.
5. Name your new asset `DA_FCroissant`.

```
+-------------------------------------------------------+
|  Content Browser: Create Data Asset                   |
+-------------------------------------------------------+
|  Right Click -> Data -> Data Asset                    |
|  Pick Class: UGSItemDataAsset                         |
|  Asset Name: DA_FCroissant                               |
+-------------------------------------------------------+
```

> [!TIP]
> **Pro Tip: Reference Existing Assets**
> If you want to inspect a working real-world example in the project at any time, open `Content/Datas/ItemsData/DA_FGlazedDonut`.

---

## Step 2: Configure Item Tags & Attributes (1 Minute)

Double-click your new `DA_FCroissant` asset to open its Inspector panel.

1. Under **Item Configuration** > **Default Tags**, click **+** to add exact project tags:
   - Add identity tag: `Item.Food.Croissant` (or `Item.Food`)
   - Add initial state tag: `State.Condition.Cook.Raw`
2. Under **Item Configuration** > **Attribute Sets**, click **+** to add attribute sets:
   - Select `UGSCookingAttributeSet` (tracks cooking progress from 0 to 100).
   - Select `UGSBurnAttributeSet` (tracks burning progress).

> [!TIP]
> **Why Tags Matter**
> Gameplay Tags act as smart hierarchical labels. World objects like stoves look for the `Item.Food` tag to accept items, while customer NPCs check for `Item.Food.Croissant` to validate customer orders.

---

## Step 3: Add Visual State Reactions (1 Minute)

When your Croissant cooks in an oven, you want its 3D model to automatically change from raw to cooked without writing tick code.

1. In the `DA_FCroissant` Inspector, locate **Item States Map**.
2. Click **+** to add a state entry and set the key tag to `State.Condition.Cook.Cooked`.
3. Under that `State.Condition.Cook.Cooked` entry:
   - Set **Max Progress Value** to `100.0`.
   - Under **Actions**, click **+** and choose **GS Item State Action Mesh Override**.
   - Assign your cooked static mesh (`StaticMesh`) to the **Mesh Override** field.
4. (Optional) Add another state entry for `State.Condition.Cook.Burned` assigning the burned static mesh.
5. Save your Data Asset.

```
+-------------------------------------------------------+
|  DA_FCroissant -> Item States Map                        |
+-------------------------------------------------------+
|  Key Tag: State.Condition.Cook.Cooked                 |
|  Actions:                                             |
|    - Mesh Override Action -> Mesh: StaticMesh (Cooked)|
+-------------------------------------------------------+
```

---

## Step 4: Place & Test in the Level (1 Minute)

To place and test the interactive Croissant in your level:

### 4.1 Set up the Physical Actor in Blueprint
1. In the Content Browser, navigate to `Content/Blueprints/Items/Foods/Cooks/`.
2. **Right-click** > **Blueprint Class** > select parent class `AGSItem` and name it `BP_FCroissant`.
3. Open `BP_FCroissant` and in the Details panel:
   - Under **Config** > **Item Data**: Assign your `DA_FCroissant` asset.
   - Under **GAS** > **Item Tags**: Add the initial state tag `State.Condition.Cook.Raw`.
   - In the **Components** panel, click **+ Add Component** and add **`GSGrabbableComponent`** (this grants the actor physical grabbing capabilities).

> [!TIP]
> **Adjusting Hand Position (`GSGrabbableComponent`)**
> If during testing the object sits too low clipping through the player's hands or floats too high, select `GSGrabbableComponent` in the Components panel and under **Details** > **Grab** > **Relative Transform**, adjust **Location Z** (e.g. `Z = 15.0`).

### 4.2 Test in the Viewport
1. Drag your `BP_FCroissant` Blueprint into your level viewport.
2. Drag a `BP_Oven` actor (located in `Content/Blueprints/Appliances/`) or `BP_GSUtilityStation_Stove` next to your item.
3. Press **Play (PIE)**! Grab the Croissant, drop it onto the stove/oven, and watch it automatically cook, increase its progress, and swap its 3D mesh at 100%.

---

## Step 5: Physical State Integration (Slow / Speed Effect & Trigger Zone)

In addition to Data-Driven items, you can create interactive physical zones (such as slow traps, freeze hazards, or speed boosts) that dynamically alter player locomotion via **Gameplay Effects (GE)** and **Mover 2.0**.

### 5.1 Create your Physical State Gameplay Effect (GE_Slowing / GE_Speed)

1. In the **Content Browser**, right-click in an empty space > **Gameplay** > **Gameplay Effect Blueprint** (or **Blueprint Class** > search `GameplayEffect`).
2. Name your asset `GE_Slowing` (or `GE_Speed`).
3. Double-click `GE_Slowing` to open its Inspector panel.
4. Under **Duration**, set **Duration Policy** to `Has Duration` and specify **Duration Magnitude** (e.g., `5.0` seconds).
5. Under **Modifiers**, click **+** to add a modifier entry:
   - **Attribute**: Select `GSMovementAttributeSet.WalkSpeed`.
   - **Modifier Op**: Select `Override` (or `Additive` / `Multiply`).
   - **Magnitude** > **Scalable Float**: Set to `200.0` (for a slow effect) or `0.0` (for a complete freeze / paralyze).
6. Save your asset.

> [!TIP]
> **How Mover 2.0 Reacts**
> You don't need to write any tick code or manual speed setters. The player pawn (`AGSPawn`) automatically listens to changes in `WalkSpeed` and updates `MoverComponent` physical speed constraints reactively!

### 5.2 Create the Trigger Zone Actor

1. In the **Content Browser**, right-click > **Blueprint Class** > **Actor**. Name it `BP_PhysicalStateTrigger`.
2. Open `BP_PhysicalStateTrigger` and add a **Box Collision** component (`BoxCollision`). Set its scale in the viewport as needed.
3. In the **Event Graph**, right-click the `BoxCollision` component > **Add Event** > **Add OnComponentBeginOverlap**.
4. Drag off the `Other Actor` pin and call **Get Ability System Component** (from `Ability System Blueprint Library` or `IAbilitySystemInterface`).
5. Drag off the returned Ability System Component pin and call **Apply Gameplay Effect To Self**.
6. Set the **Gameplay Effect Class** to your `GE_Slowing` asset.
7. Compile, save, and drag `BP_PhysicalStateTrigger` into your sandbox level. When the player pawn steps into the volume, their movement speed will change instantly for 5 seconds before returning to normal!

---

## Next Steps

Congratulations! You have created your first data-driven gameplay item and physical state zone.

* Read the full **[Creator & Editor Workflow Guide](PROJECT_DOCUMENTATION.md)** to learn how to configure customer NPCs, restaurant menus, utility stations, and monetary rewards.
* Read the **[Mover & GAS Technical Framework Guide](TECHNICAL_DOCUMENTATION.md)** if you want to dive deep into C++ architecture, physics solvers, and Verse migration patterns.
