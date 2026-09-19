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
SEARCH_FOLDER = "/Game/Items/Equipment/Weapons"

# Only touch Blueprints whose current parent is this. None reparents everything
# found, which is almost never what is wanted.
OLD_PARENT_PATH = "/Script/Somnus.SomnusWeapon"

# C++ class: "/Script/<Module>.<ClassName>". Blueprint class: the asset path with
# "_C" appended, e.g. "/Game/Blueprints/BP_Base.BP_Base_C".
NEW_PARENT_PATH = "/Script/Somnus.SomnusMeleeWeapon"

DRY_RUN = True
# ------------------------------------------------------------------------------


def parent_path_of(blueprint):
    parent = blueprint.get_editor_property("parent_class")
    return parent.get_path_name() if parent else None


def main():
    new_parent = unreal.load_object(None, NEW_PARENT_PATH)
    if new_parent is None:
        unreal.log_error("New parent not found: %s" % NEW_PARENT_PATH)
        return

    asset_lib = unreal.EditorAssetLibrary
    if not asset_lib.does_directory_exist(SEARCH_FOLDER):
        unreal.log_error("Folder not found: %s" % SEARCH_FOLDER)
        return

    changed, skipped = [], []

    for asset_path in asset_lib.list_assets(SEARCH_FOLDER, recursive=True, include_folder=False):
        asset = asset_lib.load_asset(asset_path)
        if not isinstance(asset, unreal.Blueprint):
            continue

        current = parent_path_of(asset)
        if OLD_PARENT_PATH and current != OLD_PARENT_PATH:
            skipped.append((asset_path, current))
            continue

        changed.append(asset_path)
        if DRY_RUN:
            continue

        unreal.BlueprintEditorLibrary.reparent_blueprint(asset, new_parent)
        unreal.BlueprintEditorLibrary.compile_blueprint(asset)
        asset_lib.save_loaded_asset(asset)

    unreal.log("--- %s ---" % ("DRY RUN" if DRY_RUN else "REPARENTED"))
    for path in changed:
        unreal.log("  %s" % path)
    unreal.log("%d to change, %d skipped (wrong parent)" % (len(changed), len(skipped)))
    for path, current in skipped:
        unreal.log("  skipped %s (parent %s)" % (path, current))


main()
