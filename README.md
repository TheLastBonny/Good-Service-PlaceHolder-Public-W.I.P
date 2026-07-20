# ProjectF: Good Service (Unreal Engine 5.8)

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.8-blue.svg)](https://www.unrealengine.com/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B%20%7C%20Blueprints-00599C.svg)](https://isocpp.org/)
[![Framework](https://img.shields.io/badge/Framework-Mover%20Component-orange.svg)](https://dev.epicgames.com/)
[![GAS](https://img.shields.io/badge/System-Gameplay%20Ability%20System-red.svg)](https://docs.unrealengine.com/)
[![AI](https://img.shields.io/badge/AI-StateTree-green.svg)](https://docs.unrealengine.com/)
[![Networking](https://img.shields.io/badge/Network-Network%20Prediction-purple.svg)](https://docs.unrealengine.com/)
[![Future Ready](https://img.shields.io/badge/Verse-UE%206.0%20Ready-black.svg)](https://dev.epicgames.com/)

## Overview

**ProjectF (Good Service)** is a multiplayer restaurant and service simulation framework built in **Unreal Engine 5.8**. The project serves as an architectural blueprint for modern multiplayer game design, replacing legacy engine patterns with modular, data-driven frameworks.

---

### Author's Note: The Mindset Shift Toward Data-Driven & Asynchronous Creation

> [!NOTE]
> **A Note from the Author on Architectural Philosophy**
> 
> As game creators, our ultimate goal is to let imagination flow into reality without technical friction. For years, traditional Object-Oriented Programming (OOP) in game engines forced developers into monolithic inheritance trees, endless pointer debugging, and slow iteration cycles. Creating a simple item or mechanic often meant subclassing dozens of Blueprints or fighting fragile state timers.
> 
> Game engine architecture is fundamentally shifting away from legacy OOP toward **Data-Driven Architecture (POD/DOD)** and **Asynchronous Reactive Concurrency** (such as Verse in UE 6.0 or Luau in Roblox). In this modern paradigm:
> 
> 1. **Data is Pure and Decoupled:** Objects are generic entities driven by data assets rather than rigid C++ class hierarchies.
> 2. **Asynchronous Execution Eliminates Boilerplate:** Reactive streams and concurrent primitives (`suspends`, `race`, `sync`) replace manual timer handles and CPU-heavy frame polling (`Tick`).
> 3. **Iteration Speed Surges:** Designers can build items, hazards, tools, and stations in seconds inside the editor without recompiling code.
> 
> I dove deep into the complex engineering behind Mover, GAS, StateTree, and Verse specifically to make game creation effortless for creators—so that any imaginative idea can move from concept to playable reality without technical roadblocks. This project is a living blueprint of that vision.

---

## Core Pillars

The codebase is built on three core technical pillars:
* **Mover Framework:** Constraint-based locomotion integrated with Network Prediction for local client prediction and server-authoritative reconciliation (rollback/replay).
* **Gameplay Ability System (GAS):** Attribute sets, abilities, and effects bound bidirectionally to movement, items, and world processing stations.
* **Data-Driven Entity Architecture:** Dynamic items (`AGSItem`) carrying localized ASC components driven by `UGSItemDataAsset` configuration.

---

## Documentation Portal

The documentation suite is divided into two distinct manuals below. Click on either section header to expand the embedded interactive summary, or click the direct links to read the full manuals.

<details>
<summary><b>1. Mover & GAS Technical Framework Guide (Tranek-Style Reference)</b></summary>

<br>

### Architectural Deep-Dive

Modeled after community standards such as Tranek's GAS Documentation, this guide provides modular explanations of low-level physics, networking, and ability systems:

* **Author's Notes & Paradigm Shift:** Why modern game architecture is moving from OOP to Data-Driven Design and Verse asynchronous concurrency.
* **Mover Framework Architecture:** Decoupling locomotion from `ACharacter` into modular `UBaseMovementMode` objects and `UMoverBlackboard` memory.
* **Input Production & Network Prediction:** Client prediction buffers, UDP synchronization, and authoritative server reconciliation (`FMoverSyncState::ShouldReconcile`).
* **Mover vs. Chaos Physics Coexistence:** Addressing asynchronous Physics Substepping desynchronization by implementing kinematic parabolic launches (`LaunchKinematic`).
* **Bidirectional GAS Integration:** Reactive attribute change delegates (`OnWalkSpeedChanged`) updating physical Mover speed settings dynamically.
* **Verse / UE 6.0 Paradigm Shift:** Conceptual migration paths to Verse reactive input streams, Software Transactional Memory (STM) prediction, and concurrent async coroutines (`race`, `sync`).
* **Extending the Framework:** Step-by-Step instructions for creating custom C++ Movement Modes and GAS Attribute Sets.

**[Read Full Technical Documentation](docs/TECHNICAL_DOCUMENTATION.md)**

</details>

<details>
<summary><b>2. ProjectF Creator & Editor Workflow Guide (Roblox Studio Style)</b></summary>

<br>

### Designer & Content Creator Manual

Designed with the welcoming, step-by-step workflow style of Roblox Studio documentation, this guide details how creators can build game content inside the Unreal Editor without C++ recompilation:

* **Creator's Vision:** How data-driven entities replace rigid C++ class hierarchies, unlocking creative freedom for dishes, traps, tools, and currency.
* **Creating Items Step-by-Step:** Setting up `UGSItemDataAsset`, assigning default tags, configuring localized attribute sets, and mapping state tags to visual actions (mesh overrides, sound effects, particle emitters).
* **Configuring Utility Stations:** Setting up `AGSUtilityStation` actors in levels, configuring collision volumes, socket attachments, and base vs. conditional Gameplay Effects (ovens, fryers, sinks).
* **Customer NPCs, Menus & Rewards:** Setting up level menus (`GSMenuDataAsset`), StateTree AI customer workflows, delivery validation, patience timers, and currency drops (`AGSMoneyItem`).

**[Read Full Creator Documentation](docs/PROJECT_DOCUMENTATION.md)**

</details>

---

## Architecture Flowchart

```mermaid
flowchart TD
    subgraph Client ["Client / Local Simulation"]
        Input["Player Input / UI"] --> InputProd["IMoverInputProducerInterface"]
        InputProd --> CmdCtx["FMoverInputCmdContext"]
        CmdCtx --> LocalMover["Mover Physics Solver (Predictive)"]
    end

    subgraph Network ["Network Layer (Network Prediction Plugin)"]
        CmdCtx -. UDP Sync .-> ServerMover["Server Mover Solver"]
        ServerMover -. Sync State / Rollback .-> LocalMover
    end

    subgraph Server ["Authoritative Server"]
        ServerMover --> Pawn["AGSPawn Entity"]
        Pawn <--> GAS["Gameplay Ability System (ASC)"]
        GAS <--> Attr["WalkSpeed / Health / Patience Attributes"]
        Attr --> SpeedUpdate["OnWalkSpeedChanged Callback"]
        SpeedUpdate --> ServerMover
    end

    subgraph AI ["StateTree Customer AI"]
        ST["StateTree Execution Context"] --> Task1["Assign Table"]
        ST --> Task2["Choose Random Order"]
        ST --> Task3["Send To Exit"]
        Task1 & Task2 & Task3 --> NPCComp["UGSNPCComponent"]
    end

    subgraph World ["World Interactions"]
        GAS <--> Station["AGSUtilityStation"]
        Station -- "Apply GE" --> Item["AGSItem (Local ASC)"]
        Item <--> DataAsset["UGSItemDataAsset"]
    end
```

---

## Directory Structure

```
.
├── Config/                        # Engine and Game configuration INI files
├── Content/                       # Unreal Engine Assets, Blueprints, Meshes, Audio
├── Source/
│   └── ProjectF/
│       ├── Private/               # C++ System Implementations
│       └── Public/                # C++ System Headers
├── docs/
│   ├── PROJECT_DOCUMENTATION.md   # Roblox Studio-Style Creator & Editor Guide
│   └── TECHNICAL_DOCUMENTATION.md # Tranek-Style Mover, GAS & Verse Framework Guide
└── ProjectF.uproject              # Unreal Engine 5.8 Project File
```

---

## Building the Project

### Environment Requirements
* **Unreal Engine:** 5.8
* **Compiler:** Microsoft Visual Studio 2022 (v143 toolset with C++ Game Development workload)
* **Target OS:** Windows 10/11 (64-bit)

### Build Steps
1. Clone the repository:
   ```bash
   git clone https://github.com/TheLastBonny/Good-Service-PlaceHolder-.git
   ```
2. Right-click `ProjectF.uproject` and select **Generate Visual Studio project files**.
3. Open `ProjectF.sln` in Visual Studio 2022.
4. Set build configuration to **Development Editor** and platform to **Win64**.
5. Build solution (`Ctrl + Shift + B`) and launch the editor.

---

## Document Links

* [Mover & GAS Technical Framework Guide](docs/TECHNICAL_DOCUMENTATION.md)
* [ProjectF Creator & Editor Workflow Guide](docs/PROJECT_DOCUMENTATION.md)