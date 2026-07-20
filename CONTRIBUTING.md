# Contributing to ProjectF (Good Service)

Thank you for your interest in contributing to **ProjectF (Good Service)**. 

This project is an open-source educational framework and architectural thesis for modern multiplayer game design in Unreal Engine 5.8 (featuring Mover, GAS, StateTree, and Verse migration patterns).

We welcome community feedback, issue reports, bug fixes, and feature enhancements.

---

## How to Contribute

### 1. Reporting Bugs & Suggesting Enhancements
* Open an **Issue** on the official repository.
* Provide a clear title, reproduction steps, engine version details, and error logs if applicable.

### 2. Submitting Code Changes (Pull Requests)
1. **Fork the Repository** to your own GitHub account.
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/Good-Service-PlaceHolder-.git
   ```
3. **Create a Feature Branch** off the `develop` branch:
   ```bash
   git checkout -b feature/your-feature-name origin/develop
   ```
4. **Make and Test your Changes** in Unreal Engine 5.8 / Visual Studio 2022. Ensure code compiles without warnings or breaking changes to existing C++ API contracts.
5. **Commit your Changes** with clear commit messages:
   ```bash
   git commit -m "Add feature description"
   ```
6. **Push to your Fork** and **Create a Pull Request (PR)** targeting the `develop` branch of `TheLastBonny/Good-Service-PlaceHolder-`.

---

## Code & Blueprint Standards

* **C++ Naming Conventions:** Follow Unreal Engine standard coding conventions (`AGSItem` for actors, `UGSComponent` for components, `FGSStruct` for structs, `EGSState` for enums).
* **Data-Driven Design:** Avoid hardcoding gameplay values inside C++ functions or Blueprints. Use Data Assets (`UGSItemDataAsset`, `GSMenuDataAsset`) and Gameplay Tags.
* **No Emojis or Unnecessary Assets:** Keep documentation and code comments clean, technical, and professional.

---

## Licensing Terms for Contributions

By submitting a Pull Request or contributing code/assets to this repository, you agree that your contributions will be licensed under the project's **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0)**. 

Copyright and commercial rights remain strictly with the primary author (`TheLastBonny`).
