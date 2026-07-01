# Somnus Project: AI Context Handoff

## 🚩 Current Session Overview
- **Active Task:** Phase A — Core Grid Inventory (Tarkov-style)
- **Current Step:** Step 8 — Drag & Drop UI (In Progress)
- **Primary AI Agent:** Gemini CLI (Current)

## 📝 AI Handoff Notes (Gemini -> Next Session)
- **Listen Server Broadcast Fix Complete:** All inventory functions (`TryMoveItem`, `AddItemAnywhere`, `AddItemAt`, `Internal_AddItem`, `RemoveItem`) now manually broadcast delegates for listen server players. FAS callbacks only fire on remote clients; the listen server needs manual broadcasts. No guard needed because server-side broadcasts and FAS callbacks run on different machines.
- **Drag & Drop BP Architecture Built:** `BP_ItemDragDrop` payload carries `WBP_Item`, `ItemInstance`, and `DragOffset`. `WBP_InventoryItem` handles `OnMouseButtonDown` (offset capture), `OnDragDetected` (payload creation + drag visual), and `OnDragCancelled` (opacity restore). `WBP_InventoryGrid` handles `OnDrop` (coordinate math + `Server_TryMoveItem` call).
- **InitializeGrid Pattern:** Grid UI is decoupled — `InitializeGrid(InventoryComponent)` is called externally instead of self-wiring in `Event Construct`. This allows reuse for player backpack, stash, loot containers, etc.
- **Size Box Dynamic Sizing:** Canvas Panel doesn't report desired size to parent layouts. Wrapped in a Size Box, and `InitializeGrid` sets `WidthOverride = GridWidth * TileSize`, `HeightOverride = GridHeight * TileSize`.
- **Delegate Binding in InitializeGrid:** `HandleItemAdded`, `HandleItemRemoved`, `HandleItemChanged` are bound to the component's delegates. `HandleItemChanged` uses destroy-and-recreate pattern.
- **Widget Tracking Map:** `WBP_InventoryGrid` stores a `Map<FGuid, WBP_InventoryItem>` to quickly find/destroy widgets by InstanceID.
- **NEXT ACTION:** Continue Step 8. Key remaining tasks:
  1. Implement position math in `HandleItemAdded` (`GridPosition * TileSize` for Canvas placement).
  2. Handle `bRotated` (swap widget width/height when rotated).
  3. Add hover preview during drag (green = valid, red = invalid placement).
  4. Handle OnDrop failure (return item to original position).
  5. Add right-click rotation during drag.

## ✅ Completed Tasks
- [x] Project initialization (UE 5.7, GAS, C++)
- [x] Basic folder structure for Inventory system
- [x] Definition of `USomnusItemDataAsset` base class
- [x] **Step 1:** Implemented `Size`, `ShapeMask`, `GetEffectiveSize`, and `IsCellOccupied`.
- [x] **Step 2:** Implemented `FSomnusItemInstance` and `FSomnusInventoryList` with FFastArraySerializer.
- [x] **Step 3:** Integrated `USomnusInventoryComponent` into `ASomnusCharacter`.
- [x] **Step 4 (Logic):** Implemented `RebuildOccupationGrid` (FAS aware) and `CanFitAt` (Precise collision).
- [x] **Bugfix:** Resolved `Accessed None` error by re-parenting `BP_SomnusCharacter` to refresh CDO.
- [x] **Bugfix:** Resolved `RootYawOffset` stuck at 0 by defaulting `bUseControllerRotationYaw = true` in constructor.
- [x] **Bugfix:** Resolved `UKismetMathLibrary` linker/header errors in `SomnusAnimInstance`.

- [x] **Step 5 (Logic):** Completed `AddItemAnywhere` and `AddItemAt` with robust `MaxStackCount` enforcement, partial stack merging, and leftover quantity returns.
- [x] **Step 5 (Logic):** Implemented `GetItemAt` helper utilizing precise ShapeMask boundary math (`GetEffectiveSize`) and efficient early-AABB filtering.
- [x] **Refactoring:** Cleaned up Component API (`TryRemoveItem` -> `RemoveItem`), removed obsolete ternary operators, simplified AABB math bounds.
- [x] **Step 5 (Move Item):** Implement `TryMoveItem` logic (the backbone of the drag-and-drop action).
- [x] **Step 7 (Delegates):** Set up `FOnInventoryItemChanged` multicast delegates to sync UI with C++ data.
- [x] **Step 6 (UI Foundation):** Created `WBP_InventoryGrid`, `WBP_InventoryItem`, and `WBP_GridCell` (Canvas Panel math is working for empty cells).
- [x] **Starter Kit (BeginPlay):** Added `DefaultItems` to the inventory component and implemented `BeginPlay()` to grant items.
- [x] **Bugfix:** Listen server FAS broadcast — added manual delegate broadcasts for listen server players across all inventory mutation functions.

## 🛠 Active Work-in-Progress
- [ ] **Step 8 (Drag & Drop UI):** BP payload, drag detection, and OnDrop coordinate math are wired up. Remaining: canvas positioning, rotation handling, hover preview, failure recovery.
- **Tech Debt (Item Data Asset):** In the future, we need to create a Slate Custom Detail Customization for `USomnusItemDataAsset` to show `ShapeMask` as a 2D grid instead of a 1D array.

## 🔜 Next Steps (Immediate)
- [ ] **Canvas Placement:** `HandleItemAdded` needs `GridPosition * TileSize` to set widget position on Canvas.
- [ ] **Rotation Visual:** Swap widget Width/Height when `bRotated` is true.
- [ ] **Hover Preview:** Show green/red tint on grid cells during drag to indicate valid/invalid placement.
- [ ] **Drop Failure:** If `Server_TryMoveItem` returns false (or move fails), restore item to original position.
- [ ] **Right-Click Rotate:** Allow rotation during drag via right-click.

## 📌 Technical Context & Rules
- **Rotation:** Character Actor rotates with Controller (`bUseControllerRotationYaw = true`). Visual counter-rotation handled by AnimBP via `RootYawOffset`.
- **Grid Math:** Top-Left is (0,0). Boundary checks use `TopLeft + ItemSize - 1` to avoid off-by-one errors.
- **Discipline:** Study Mode active. User types core logic; AI provides architecture/boilerplate.
- **Debug:** `PrintDebugGrid()` logs a visual ASCII representation of the TBitArray.

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
Use `UPROPERTY(NotReplicated)` with `TObjectPtr<class USomnusInventoryComponent>`. This tells UE's GC
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
