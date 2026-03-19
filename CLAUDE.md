# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Project Somnus is an Unreal Engine 5.7 C++ action game built on the Gameplay Ability System (GAS). Single runtime module "Somnus" with multiplayer-ready architecture.

## Build Commands

```bash
# Build from command line (Development)
"C:/Program Files/Epic Games/UE_5.7/Engine/Build/BatchFiles/Build.bat" SomnusEditor Win64 Development -Project="C:/Dev/Unreal Projects/Somnus/Somnus.uproject" -WaitMutex -FromMsBuild

# Generate project files
"C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe" -projectfiles -project="C:/Dev/Unreal Projects/Somnus/Somnus.uproject" -game -rocket -progress
```

No automated tests or linting are configured.

## Module Dependencies (Somnus.Build.cs)

Public: Core, CoreUObject, Engine, InputCore, EnhancedInput, GameplayAbilities, GameplayTags, GameplayTasks, AnimGraphRuntime

When adding new GAS-related classes, these modules are already available. Add new module dependencies to `Source/Somnus/Somnus.Build.cs`.

## Architecture

### GAS Ownership Pattern
The **AbilitySystemComponent (ASC) lives on PlayerState**, not the Character. This is the recommended pattern for multiplayer action games:
- `ASomnusPlayerState` owns the ASC and `USomnusAttributeSet`
- `ASomnusCharacter` implements `IAbilitySystemInterface` but delegates to PlayerState's ASC
- ASC uses **Mixed replication mode** (effects replicate only to owning client)
- Character initializes ASC in both `PossessedBy()` (server) and `OnRep_PlayerState()` (client)

### Data-Driven Weapon System
Weapons are **configured entirely in Blueprints** — no hardcoded ability logic in C++:
- `ASomnusWeapon` — base class with `AbilitiesToGrant` array and `WeaponTags` container
- `ASomnusMeleeWeapon` — extends base with trace socket names and radius for hit detection
- `Equip()` grants abilities + tags to ASC; `Unequip()` revokes them
- Weapon equip is server-authoritative; visual attachment replicates via `OnRep_OwningCharacter()`

### Animation-Driven Hit Detection
Melee hit detection is driven by animation notify states, not tick:
- `USomnusAnimNotifyState_MeleeTrace` performs sphere traces between weapon base/tip sockets each tick of the notify window
- Hits are sent as `Event.Melee.Hit` gameplay events to the character's ASC
- Melee abilities should listen for this gameplay event tag to apply damage

### Gameplay Tags (SomnusGameplayTags.h)
All project tags are declared in `SomnusGameplayTags` namespace. Add new tags there — do not use raw FName strings for tags elsewhere in C++.

### Lyra-Style Input System
- `USomnusInputConfig` — data asset mapping `UInputAction` → `FGameplayTag` for both native (Move/Look/Jump) and ability inputs
- `USomnusInputComponent` — extends `UEnhancedInputComponent` with template `BindNativeAction` and `BindAbilityActions` methods
- Character holds an `InputTagToAbilityTags` map (configured in BP) to route input tags to ability tags
- Third-person camera (spring arm at 400 units)
- Character orients rotation to movement (no strafing by default)
- `USomnusAnimInstance` tracks locomotion state (gait, direction, ground speed, jump/fall state) with direction hysteresis to prevent flickering

### HUD Integration
- `InitHUD()` and `UpdateHealthUI()`/`UpdateStaminaUI()` are `BlueprintImplementableEvent` — UI is built in Blueprints
- Attribute change delegates on `USomnusAttributeSet` drive UI updates

## Source Layout

```
Source/Somnus/
├── AbilitySystem/Attributes/   # Attribute sets (Health, Stamina)
├── Animation/                  # AnimInstance + AnimNotifies
├── Character/                  # Player character
├── Core/                       # GameMode, PlayerState, GameplayTags
├── Equipment/                  # Weapon base + melee weapon
└── Input/                      # InputConfig data asset + InputComponent
```

## Key Conventions

- Prefix all classes with `Somnus` (e.g., `ASomnusCharacter`, `USomnusAttributeSet`)
- Use `ATTRIBUTE_ACCESSORS` macro for all GAS attributes
- Server-authoritative gameplay: use `HasAuthority()` checks before mutating replicated state
- Weapon/equipment properties use `EditDefaultsOnly` for Blueprint configuration
- Replicated properties use `OnRep_*` callbacks for client-side visual sync

## Enabled Plugins

GameplayAbilities, AnimationLocomotionLibrary, AnimationWarping — these are already configured in Somnus.uproject.
