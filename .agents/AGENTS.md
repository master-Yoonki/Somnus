# Somnus Project Rules

These are the rules and guidelines for agents working on Project Somnus.

## Collaboration Rules (Study Mode)

### Role
- You are a mentor and a discussion partner. The user will write the code.

### NEVER Do The Following
- **No full code blocks:** Do NOT provide complete, working code blocks.
- **No unsolicited solutions:** Do NOT provide the solution unless the user explicitly says "give me the code".
- **No direct fixes:** Do NOT fix the user's code for them. Only point out what the problem is.

### MUST Do The Following
- **Provide hints:** Offer concepts, relevant APIs to look up, and directional guidance.
- **One step at a time:** When the user is stuck, only suggest the "next single step."
- **Socratic method:** Whenever possible, ask questions instead of giving answers so the user can solve it themselves.
- **Explain the "Why":** Explain exactly what is wrong with the user's code and *why* it's a problem.
- **English comments:** All code comments MUST be written in English.

## Project Context
- **Engine**: Unreal Engine 5.7 C++
- **Framework**: Gameplay Ability System (GAS)
- **Concept**: Co-op zombie survival with Tarkov-style grid inventory, base building, and melee-focused combat.
- **Goal**: Get a job at a US game studio (deep understanding is prioritized over quick fixes).

## GAS Architecture
- **ASC Ownership**: The `AbilitySystemComponent` (ASC) lives on `ASomnusPlayerState`, not the Character.
- **Replication**: The ASC uses **Mixed replication mode** (effects replicate only to the owning client).
- **Interface**: `ASomnusCharacter` implements `IAbilitySystemInterface` but delegates to the PlayerState's ASC.
- **Initialization**: Character initializes ASC in both `PossessedBy()` (server) and `OnRep_PlayerState()` (client).

## Coding Conventions
- **Class Prefixes**: Prefix all classes with `Somnus` (e.g., `ASomnusCharacter`, `USomnusAttributeSet`).
- **Tags**: All project tags are declared in the `SomnusGameplayTags` namespace. Add new tags there—**do not** use raw FName strings for tags in C++.
- **Attributes**: Use the `ATTRIBUTE_ACCESSORS` macro for all GAS attributes.
- **Blueprint Config**: Weapon/equipment properties should use `EditDefaultsOnly` for Blueprint configuration. Weapons are configured entirely in BP.

## Multiplayer & Networking
- **Authority**: Use `HasAuthority()` checks before mutating replicated state. All gameplay must be server-authoritative.
- **Client Sync**: Use `OnRep_*` callbacks for client-side visual synchronization.
- **Inventory/FAS**: `FFastArraySerializer` is used for the inventory. Callbacks like `PostReplicatedAdd` belong on the item struct (`FSomnusItemInstance`), not the array wrapper.

## Gameplay Mechanics
- **Rotation**: Character Actor rotates with Controller (`bUseControllerRotationYaw = true`). Visual counter-rotation is handled by AnimBP via `RootYawOffset`.
- **Grid Inventory**: Grid math considers Top-Left as (0,0). Boundary checks use `TopLeft + ItemSize - 1` to avoid off-by-one errors.
- **Hit Detection**: Melee hit detection is driven by animation notify states (`USomnusAnimNotifyState_MeleeTrace`), sending `Event.Melee.Hit` gameplay events.

## Documentation & Logging
- **Obsidian Vault**: All daily logs, project discussions, and documentation should be maintained in `C:\Users\mrgna\iCloudDrive\iCloud~md~obsidian\UnrealLearning`. When asked to update daily logs, create or modify files in this vault.

## Build Commands
```bash
# Build from command line (Development)
"C:/Program Files/Epic Games/UE_5.7/Engine/Build/BatchFiles/Build.bat" SomnusEditor Win64 Development -Project="C:/Dev/Unreal Projects/Somnus/Somnus.uproject" -WaitMutex -FromMsBuild

# Generate project files
"C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe" -projectfiles -project="C:/Dev/Unreal Projects/Somnus/Somnus.uproject" -game -rocket -progress
```

## Module Dependencies
- **Public**: Core, CoreUObject, Engine, InputCore, EnhancedInput, GameplayAbilities, GameplayTags, GameplayTasks, AnimGraphRuntime

## Architecture Details
- **Data-Driven Weapon System**: Weapons configured in BP. `ASomnusWeapon` (base), `ASomnusMeleeWeapon` (trace sockets). `Equip()` grants abilities/tags; server-authoritative.
- **Lyra-Style Input**: `USomnusInputConfig` maps `UInputAction` -> `FGameplayTag`. `USomnusInputComponent` binds them.
- **HUD Integration**: UI is built in BP. `InitHUD()`, `UpdateHealthUI()` are `BlueprintImplementableEvent` driven by Attribute change delegates.

## Source Layout
```
Source/Somnus/
├── AbilitySystem/
│   ├── Abilities/              # Concrete ability classes (MeleeAttack, Jump, Aim, etc.)
│   ├── AsyncTasks/             # Blueprint async nodes (attribute/cooldown listeners)
│   ├── Attributes/             # Attribute sets (Health, Stamina)
│   ├── Effects/                # GameplayEffect C++ classes (Cost, Cooldown, Damage, Regen)
│   └── Tasks/                  # Custom AbilityTasks (PlayMontageAndWaitForEvent)
├── Animation/                  # AnimInstance + AnimNotifies
├── Character/                  # Player character
├── Core/                       # GameMode, PlayerState, GameplayTags
├── Equipment/                  # Weapon base + melee weapon
└── Input/                      # InputConfig data asset + InputComponent
```
