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

### 1. Open the **Content Browser** in Unreal Editor.

<img width="1870" height="653" alt="Captura de pantalla 2026-08-12 094020" src="https://github.com/user-attachments/assets/f009a96f-093f-4296-b9e1-ae4521e846a4" />




### 2. Navigate to `Content/Datas/ItemsData/`.
   
<img width="1882" height="693" alt="Captura de pantalla 2026-08-12 094306" src="https://github.com/user-attachments/assets/c99c6ebd-8844-4817-aef7-11d9a6b1b02e" />




### 3. **Right-click** in an empty space and select **Data** > **Data Asset**.
   
<img width="1919" height="1021" alt="image" src="https://github.com/user-attachments/assets/9bae7a6b-7ac2-4a5c-8751-06dcf0e002d5" />




### 4. In the class picker search bar, type `GSItemDataAsset`, select it, and click **Select**.

<img width="770" height="505" alt="Captura de pantalla 2026-08-12 094231" src="https://github.com/user-attachments/assets/d35db05a-625b-4799-86ef-50dbd75d7b89" />




### 5. Name your new asset `DA_FCroissant`.

<img width="480" height="252" alt="Captura de pantalla 2026-08-12 094413" src="https://github.com/user-attachments/assets/4229d242-cf93-4ded-8ee5-c2a07e878580" />




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

<img width="1150" height="305" alt="Captura de pantalla 2026-08-12 094423" src="https://github.com/user-attachments/assets/9367a806-8fe6-4864-bee6-8e7b98b54ed5" />


1. Under **Item Configuration** > **Default Tags**, click **+** to add exact project tags:
   - Add identity tag: `Item.Food.Croissant` (or `Item.Food`)
<img width="1165" height="771" alt="image" src="https://github.com/user-attachments/assets/a4a4bf75-2fd9-4b19-b0e0-2cd9a09dfe82" />
<img width="1231" height="733" alt="image" src="https://github.com/user-attachments/assets/dcaeb9f0-201f-47a7-b2b9-f7607dcd0cf1" />

     
2. Under **Item Configuration** > **Attribute Sets**, click **+** to add attribute sets:
   - Select `UGSCookingAttributeSet` (tracks cooking progress).
   - Select `UGSBurnAttributeSet` (tracks burning progress).

<img width="1172" height="258" alt="image" src="https://github.com/user-attachments/assets/4b0cdb28-23c1-4757-8aa6-aa282987bf11" />


> [!TIP]
> **Why Tags Matter**
> Gameplay Tags act as smart hierarchical labels. World objects like stoves look for the `Item.Food` tag to accept items, while customer NPCs check for `Item.Food.Croissant` to validate customer orders.

---

## Step 3: Add Visual State Reactions (1 Minute)

When your Croissant cooks in an oven, you want its 3D model to automatically change from raw to cooked without writing tick code.

1. In the `DA_FCroissant` Inspector, locate **Item States Map**.
2. Click **+** to add a state entry and set the key tag to `State.Condition.Cook.Raw`
3. Under that `State.Condition.Cook.Raw` entry:
4.  - Set **Max Progress Attribute** to `None`.
5.  - Set **Max Progress Value** to `0.0`.
6.  - Under **Actions**, click **+** and choose **Mesh Override**.
7.  - Assign your raw static mesh (`StaticMesh`) to the **Mesh Override** field.
8. **Now, we repeat the process for Cooked:**
9. In the `DA_FCroissant` Inspector, locate **Item States Map**.
10. Click **+** to add a state entry and set the key tag to `State.Condition.Cook.Cooked`
11. Under that `State.Condition.Cook.Cooked` entry:
   - Set **Max Progress Attribute** to `GSCookingAttributeSet.MaxCookingProgress`.
   - Set **Max Progress Value** to `100.0`.
   - Under **Actions**, click **+** and choose **Mesh Override**.
   - Assign your cooked static mesh (`StaticMesh`) to the **Mesh Override** field.

<img width="1517" height="667" alt="Captura de pantalla 2026-08-12 094819" src="https://github.com/user-attachments/assets/c2df1965-f6ec-487e-a803-906346852b5e" />


12. **Repeat the process for Burned:** (Opcional)
13. Click **+** to add a state entry and set the key tag to `State.Condition.Cook.Burned`
14. Under that `State.Condition.Cook.Burned` entry:
   - Set **Max Progress Attribute** to `GSCookingAttributeSet.MaxBurnProgress`.
   - Set **Max Progress Value** to `200.0`.
   - Under **Actions**, click **+** and choose **Mesh Override**.
   - Assign your burned static mesh (`StaticMesh`) to the **Mesh Override** field.
15. Save your Data Asset.

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
1. In the Content Browser, navigate to `Content/Blueprints/Items/Foods`.
2. **Right-click** > **Blueprint Class** > select parent class `AGSItem` and name it `BP_FCroissant`.

<img width="1914" height="1020" alt="image" src="https://github.com/user-attachments/assets/96b8a3a7-76b0-4bcb-9320-a39c12503e00" />
<img width="1918" height="946" alt="image" src="https://github.com/user-attachments/assets/31c9e1a4-4fdc-48e4-9618-159071a563e2" />


4. Open `BP_FCroissant` and in the Details panel:
   - Under **Config** > **Item Data**: Assign your `DA_FCroissant` asset.
   - Under **GAS** > **Item Tags**: Add the initial state tag `State.Condition.Cook.Raw`.
  
     <img width="1918" height="1017" alt="image" src="https://github.com/user-attachments/assets/d901844c-8011-4d54-b42a-c050978b6ea0" />
     <img width="637" height="243" alt="image" src="https://github.com/user-attachments/assets/104efa53-6926-4568-938a-5d8b7f18f0ba" />


   - In the **Components** panel, click **+ Add Component** and add **`GSGrabbableComponent`** (this grants the actor physical grabbing capabilities).
  
     <img width="1262" height="642" alt="image" src="https://github.com/user-attachments/assets/31791f2a-6342-4a09-9697-e08e9f42fa4f" />

   - Also, add a static mesh in the Inspector and assign your object's default mesh to it.
  
     <img width="526" height="608" alt="image" src="https://github.com/user-attachments/assets/dfba0e87-855a-4685-a20b-6569e31df1b8" />
     <img width="1918" height="852" alt="image" src="https://github.com/user-attachments/assets/eae80d96-b557-40e9-9359-6f80b31fc737" />
     <img width="1912" height="866" alt="image" src="https://github.com/user-attachments/assets/b13b52e1-d8e0-41a9-885e-81aab364bdc1" />


> [!TIP]
> **Adjusting Hand Position (`GSGrabbableComponent`)**
> If during testing the object sits too low clipping through the player's hands or floats too high, select `GSGrabbableComponent` in the Components panel and under **Details** > **Grab** > **Relative Transform**, adjust **Location Z** (e.g. `Z = 15.0`).

### 4.2 Test in the Viewport
1. Drag your `BP_FCroissant` Blueprint into your level viewport.
2. Drag a `BP_Oven` actor (located in `Content/Blueprints/Appliances/`) or `BP_GSUtilityStation_Stove` next to your item.
3. Press **Play (PIE)**! Grab the Croissant, drop it onto the stove/oven, and watch it automatically cook, increase its progress, and swap its 3D mesh at 100%.

<img width="1918" height="1017" alt="image" src="https://github.com/user-attachments/assets/d6c3bf3a-1be4-44f3-8a63-c0f97fc39d98" />
<img width="1387" height="865" alt="image" src="https://github.com/user-attachments/assets/13a7875b-2eb8-4fd0-860b-37c6e76490db" />
<img width="1382" height="822" alt="image" src="https://github.com/user-attachments/assets/1ce08186-e702-4eac-b88b-049f61138ca7" />

---

## Step 5: Physical State Integration (Slow / Speed Effect & Trigger Zone)

In addition to Data-Driven items, you can create interactive physical zones (such as slow traps, freeze hazards, or speed boosts) that dynamically alter player locomotion via **Gameplay Effects (GE)** and **Mover 2.0**.

### 5.1 Create your Physical State Gameplay Effect (GE_Slowing / GE_Speed)

1. In the **Content Browser** Go towards `Content/GAS/GE`, right-click in an empty space > **Gameplay** > **Gameplay Effect Blueprint** (or **Blueprint Class** > search `GameplayEffect`).
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

1. In the **Content Browser** Go towards `Content/Blueprints`, right-click > **Blueprint Class** > **Actor**. Name it `BP_PhysicalStateTrigger`.
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
