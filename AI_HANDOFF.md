# Somnus Project: AI Context Handoff

## 🚩 Current Session Overview
- **Active Task:** Phase A — Core Grid Inventory (Tarkov-style)
- **Current Step:** Step 1 — Item Data Asset (Size + ShapeMask)
- **Primary AI Agent:** Gemini CLI (Current) / Claude Code (Standby)

## ✅ Completed Tasks
- [x] Project initialization (UE 5.7, GAS, C++)
- [x] Basic folder structure for Inventory system
- [x] Definition of `USomnusItemDataAsset` base class
- [x] **Step 1:** Implemented `Size`, `ShapeMask`, `GetEffectiveSize`, and `IsCellOccupied`.
- [x] **Step 2:** Implemented `FSomnusItemInstance` and `FSomnusInventoryList` with FFastArraySerializer.
- [x] **Step 3:** Integrated `USomnusInventoryComponent` into `ASomnusCharacter` (C++ implementation complete).
- [x] **Bugfix:** Resolved `NetCore` linker errors and `OutLifetimeProps` macro naming.

## 🛠 Active Work-in-Progress & Blocker
- **Blocker (Step 3):** Blueprint `Accessed None` error.
  - **Symptoms:** `BP_SomnusCharacter` fails to access the `Inventory` component despite the C++ constructor logging `Inventory Component Initialized`.
  - **Susppected Cause:** Unreal Editor CDO/Blueprint synchronization issue after modifying the C++ constructor while the editor was open.
  - **Action taken:** Verified `CreateDefaultSubobject` logic and `GetInventory()` accessor. Re-parenting or hard restart (Binaries delete) suggested.
- **Step 4 (Scaffolded):** `OccupationGrid` (TBitArray) initialized in `InitializeComponent`. Stubs for `CanFitAt`, `FindFirstFit`, and `RebuildOccupationGrid` added to C++.

## 🔜 Next Steps (Immediate)
1. **Fix:** Resolve the Blueprint `Accessed None` error (Restart Editor, Re-parent BP, or Delete Binaries/Intermediate).
2. **Implementation:** Move inline callback stubs in `SomnusItemInstance.h` to `.cpp` (if not already done) to wire logic.
3. **Step 4:** Implement core grid math in `SomnusInventoryComponent.cpp`.
4. **Step 5:** Implement server-authoritative mutation methods (`TryAddItem`).

## 📌 Technical Context & Rules
- **Coordinate System:** (0,0) is Top-Left. X is Width (Columns), Y is Height (Rows).
- **Networking:** Lyra-style Item-level callbacks.
- **Discipline:** Study Mode active. User types core logic; AI provides architecture/boilerplate.
- **Debug:** `PrintDebugGrid()` is available in C++ and BP for grid visualization.

---

## 🔍 Step 2 Review Context (for Gemini)

### What was designed (Claude Code review, 2026-04-30)

**Fast Array Serializer (FAS) — critical correction:**
Gemini's earlier task list placed `PostReplicatedAdd`, `PostReplicatedChange`, and `PreReplicatedRemove`
on `FSomnusInventoryList`. **This is wrong.** These callbacks belong on `FSomnusItemInstance`
(the item struct, not the container). `FFastArraySerializerItem` declares them as virtual stubs;
Unreal's delta-serialization loop calls them on each changed item. The list only needs
`NetDeltaSerialize` and the `OwnerComponent` back-pointer.

**OwnerComponent on the list:**
Use `UPROPERTY(NotReplicated)` with `TObjectPtr<USomnusInventoryComponent>`. This tells UE's GC
to track the pointer without serializing it over the network. It is set locally post-construction
(by the component on both server and client), never sent over the wire.

**ItemGuid generation:**
The helper constructor generates the GUID via `FGuid::NewGuid()`. This is correct because only
the server ever constructs item instances (server-authoritative). The GUID replicates to clients
as a normal member of the delta-serialized struct. Clients never call the constructor.

**TStructOpsTypeTraits placement:**
The specialization must go OUTSIDE both struct definitions, at file scope, after
`FSomnusInventoryList` is fully defined. Without `WithNetDeltaSerializer = true`, UE falls back
to full-array replication silently — no compile error, just wrong behavior.

### Expected final shape of SomnusItemInstance.h
```
FSomnusItemInstance : FFastArraySerializerItem
  Members: ItemData, Quantity, GridPosition, bIsRotated, ItemGuid
  Callbacks: PostReplicatedAdd, PostReplicatedChange, PreReplicatedRemove
             (each takes const FSomnusInventoryList&)

FSomnusInventoryList : FFastArraySerializer
  Members: Items (TArray<FSomnusItemInstance>), OwnerComponent (NotReplicated)
  Methods: NetDeltaSerialize (one-liner calling FastArrayDeltaSerialize)

TStructOpsTypeTraits<FSomnusInventoryList> — WithNetDeltaSerializer = true
```

### Step 2 review — fixes applied (Claude Code, 2026-05-01)
These issues were found and corrected. **Do not re-introduce them.**

| Issue | Was | Fixed to |
|---|---|---|
| `OwnerComponent` UPROPERTY specifier | `VisibleAnywhere, BlueprintReadOnly` | `NotReplicated` |
| `OwnerComponent` pointer type | raw `USomnusInventoryComponent*` | `TObjectPtr<class USomnusInventoryComponent>` |
| Item member specifiers | `VisibleAnywhere, BlueprintReadOnly` | `BlueprintReadOnly` only |
| `FSomnusInventoryList` USTRUCT | `USTRUCT(BlueprintType)` | `USTRUCT()` — not a BP-facing type |
| `Items` array specifiers | `VisibleAnywhere, BlueprintReadOnly` | `UPROPERTY()` — FAS handles its own serialization |
| `OnItemRemoved` log | no item name logged | logs item name (consistent with Add/Changed) |

### Current state of SomnusItemInstance.h (verified correct)
```
FSomnusItemInstance : FFastArraySerializerItem  [USTRUCT(BlueprintType)]
  Members: ItemData, StackCount, CurrentDurability, GridPosition, bRotated, InstanceID
  Callbacks (inline stubs, will move to .cpp in Step 6):
    PreReplicatedRemove, PostReplicatedAdd, PostReplicatedChange
    (each takes const FSomnusInventoryList&)

FSomnusInventoryList : FFastArraySerializer  [USTRUCT() — no BlueprintType]
  UPROPERTY() TArray<FSomnusItemInstance> Items
  UPROPERTY(NotReplicated) TObjectPtr<USomnusInventoryComponent> OwnerComponent
  NetDeltaSerialize — one-liner calling FastArrayDeltaSerialize<FSomnusItemInstance, FSomnusInventoryList>

TStructOpsTypeTraits<FSomnusInventoryList> — WithNetDeltaSerializer = true  [file scope, outside structs]
```

### Naming note
User chose `InstanceID` (not `InstanceId`). Either is fine — just stay consistent.
If Gemini introduces new ID fields elsewhere, match this casing.
