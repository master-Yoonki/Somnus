"""Reparent every Blueprint under a folder to a new parent class.

Reparenting drops anything the old parent gave a Blueprint that the new one does
not: variables, components, implemented interfaces, and overridden functions. The
Blueprint keeps compiling, so the loss is quiet - run with DRY_RUN first and read
the list before letting it write.

Run from the editor: Tools > Execute Python Script, or `py <path to this file>`.
Close every open Blueprint editor first; an asset open for edit is reparented
underneath the window and saves whichever copy loses the race.
"""

import unreal

# ---------------------------------------------------------------- configuration
SEARCH_FOLDER = "/Game/ShoppingMall/Blueprints/DoorBP"

# Only touch Blueprints whose current parent is this. None accepts any parent,
# which is what a folder of assets from someone else usually needs - the dry run
# prints what each one is inheriting from now so the list can be read first.
OLD_PARENT_PATH = None

# C++ class: "/Script/<Module>.<ClassName>". Blueprint class: the asset path with
# "_C" appended, e.g. "/Game/Blueprints/BP_Base.BP_Base_C".
NEW_PARENT_PATH = "/Game/ShoppingMall/Blueprints/DoorBP/BP_DoorBase.BP_DoorBase_C"

# The new parent itself lives in the search folder, and a class cannot inherit
# from itself. Anything else to leave alone goes here too.
EXCLUDE_ASSETS = [
    "/Game/ShoppingMall/Blueprints/DoorBP/BP_DoorBase.BP_DoorBase",
]

DRY_RUN = True
# ------------------------------------------------------------------------------


def normalize_class_path(value):
    """Registry tags carry class references in export-text form - the tag reads
    /Script/CoreUObject.Class'/Script/Engine.Actor', not the bare path."""
    if not value:
        return None
    text = str(value)
    return text.split("'")[1] if "'" in text else text


def blueprints_in(folder):
    """Ask the registry rather than EditorAssetLibrary.list_assets, which returns
    the generated class alongside every Blueprint and doubles the list."""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    for data in registry.get_assets_by_path(folder, recursive=True):
        if data.asset_class_path.asset_name != "Blueprint":
            continue
        object_path = "%s.%s" % (data.package_name, data.asset_name)
        yield object_path, normalize_class_path(data.get_tag_value("ParentClass"))


def main():
    new_parent = unreal.load_object(None, NEW_PARENT_PATH)
    if new_parent is None:
        unreal.log_error("New parent not found: %s" % NEW_PARENT_PATH)
        return
    new_parent_path = new_parent.get_path_name()

    changed, skipped = [], []

    for object_path, current in sorted(blueprints_in(SEARCH_FOLDER)):
        if object_path in EXCLUDE_ASSETS:
            skipped.append((object_path, "excluded, parent is %s" % current))
            continue
        if current == new_parent_path:
            skipped.append((object_path, "already parented"))
            continue
        if OLD_PARENT_PATH and current != OLD_PARENT_PATH:
            skipped.append((object_path, "parent is %s" % current))
            continue

        changed.append((object_path, current))
        if DRY_RUN:
            continue

        asset = unreal.EditorAssetLibrary.load_asset(object_path)
        unreal.BlueprintEditorLibrary.reparent_blueprint(asset, new_parent)
        unreal.BlueprintEditorLibrary.compile_blueprint(asset)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)

    unreal.log("--- %s -> %s ---" % ("DRY RUN" if DRY_RUN else "REPARENTED", new_parent_path))
    for object_path, current in changed:
        unreal.log("  %s   (was %s)" % (object_path, current))
    unreal.log("%d to change, %d skipped" % (len(changed), len(skipped)))
    for object_path, reason in skipped:
        unreal.log("  skipped %s (%s)" % (object_path, reason))


main()
