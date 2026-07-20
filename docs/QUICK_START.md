# 5-Minute Creator Quick Start Guide

Welcome to the **5-Minute Creator Quick Start Guide** for **ProjectF (Good Service)** in Unreal Engine 5.8!

This guide is designed for level designers, content creators, and newcomers. In less than 5 minutes, you will learn how to create a brand new interactive item (a Steak), set up visual cooking states, and test it on a cooking station—**without writing a single line of C++ or Blueprint code**.

---

## Traditional Game Development vs. ProjectF

Before jumping in, consider how game creation changes with a Data-Driven Workflow:

| Workflow Action | Traditional Engine Approach (OOP) | ProjectF (Data-Driven Workflow) |
| :--- | :--- | :--- |
| **Creating 10 new items** | Create 10 new Blueprint classes + complex inheritance | Create 10 simple Data Assets in the Content Browser |
| **Setup Time per Item** | 30 to 60 minutes of Blueprint scripting | 2 minutes in the Inspector panel |
| **Asset Maintenance** | 50+ duplicated Blueprint files to manage | 1 single generic actor + data files |
| **Code Recompilation** | Required after C++ structural changes | Zero code recompilation required |

---

## Step 1: Create your First Item Data Asset (2 Minutes)

Imagine every interactive object in your game—food, tools, traps, or money—sharing a single, highly optimized physical actor. You never need to create a new Blueprint class for a new item; you simply create a **Data Asset**.

1. Open the **Content Browser** in Unreal Editor.
2. Navigate to `Content/DataAssets/Items/` (or create a new folder).
3. **Right-click** in an empty space and select **Miscellaneous** > **Data Asset**.
4. In the class picker search bar, type `GSItemDataAsset`, select it, and click **Select**.
5. Name your new asset `DA_Steak`.

```
+-------------------------------------------------------+
|  Content Browser: Create Data Asset                   |
+-------------------------------------------------------+
|  Pick Class: UGSItemDataAsset                         |
|  Asset Name: DA_Steak                                 |
+-------------------------------------------------------+
```

---

## Step 2: Configure Item Tags & Attributes (1 Minute)

Double-click your new `DA_Steak` asset to open its Inspector panel.

1. Under **Item Configuration** > **Default Tags**, click **+** to add tags:
   - Add identity tag: `Food.Steak`
   - Add initial state tag: `State.Raw`
2. Under **Item Configuration** > **Attribute Sets**, click **+** to add attribute sets:
   - Select `GSCookingAttributeSet` (tracks cooking progress from 0 to 100).
   - Select `GSBurnAttributeSet` (tracks burning progress).

> [!TIP]
> **Why Tags Matter**
> Gameplay Tags act as smart labels. World objects like stoves look for the `Food` tag to accept items, while NPCs check for `Food.Steak` to validate customer orders.

---

## Step 3: Add Visual State Reactions (1 Minute)

When your steak cooks on a stove, you want its 3D model to automatically change from raw to cooked without writing tick code.

1. In the `DA_Steak` Inspector, locate **Item States Map**.
2. Click **+** to add a state entry and set the key tag to `State.Cooked`.
3. Under **Actions**, click **+** and choose **GS Item State Action Mesh Override**.
4. Assign your cooked static mesh (`SM_Steak_Cooked`) to the **Mesh Override** field.
5. Save your Data Asset.

```
+-------------------------------------------------------+
|  DA_Steak -> Item States Map                          |
+-------------------------------------------------------+
|  Key Tag: State.Cooked                                |
|  Actions:                                             |
|    - Mesh Override Action -> Mesh: SM_Steak_Cooked    |
|    - Sound Action         -> Sound: SFX_Sizzle        |
+-------------------------------------------------------+
```

---

## Step 4: Place & Test in the Level (1 Minute)

1. Drag the base `BP_GSItem` actor into your level viewport.
2. In the Details Panel, find **Item Configuration** > **Item Data**.
3. Assign your newly created `DA_Steak` asset to the field.
4. Drag a `BP_GSUtilityStation_Stove` actor next to your item.
5. Press **Play (PIE)**! Grab the steak, drop it onto the stove, and watch it automatically cook, swap its 3D mesh, and sizzle in real time.

---

## Next Steps

Congratulations! You have created your first data-driven gameplay item.

* Read the full **[Creator & Editor Workflow Guide](PROJECT_DOCUMENTATION.md)** to learn how to configure customer NPCs, restaurant menus, utility stations, and monetary rewards.
* Read the **[Mover & GAS Technical Framework Guide](TECHNICAL_DOCUMENTATION.md)** if you want to dive deep into C++ architecture, physics solvers, and Verse migration patterns.
