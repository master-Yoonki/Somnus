import unreal

ROOT_PATH = "/Game/Characters"
registry = unreal.AssetRegistryHelpers.get_asset_registry()
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

all_assets = registry.get_assets_by_path(ROOT_PATH, recursive=True)
dupes = [a for a in all_assets if str(a.asset_name).endswith("_Retargeted1")]

pairs = []
for d in dupes:
    dname = str(d.asset_name)
    base_name = dname[:-1]
    old_path = f"{d.package_path}/{base_name}.{base_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(old_path):
        pairs.append((old_path, f"{d.package_name}.{dname}", str(d.package_path), base_name))

unreal.log(f"PAIRS: {len(pairs)}")

# Load everything in scope so references get fixed up during consolidate
all_paths = [f"{a.package_name}.{a.asset_name}" for a in all_assets]
for p in all_paths:
    unreal.EditorAssetLibrary.load_asset(p)

ok_count = 0
fail_count = 0
for old_path, new_path, folder, base_name in pairs:
    old_asset = unreal.EditorAssetLibrary.load_asset(old_path)
    new_asset = unreal.EditorAssetLibrary.load_asset(new_path)
    if not old_asset or not new_asset:
        unreal.log_error(f"Failed to load pair: {old_path} / {new_path}")
        fail_count += 1
        continue
    ok = unreal.EditorAssetLibrary.consolidate_assets(new_asset, [old_asset])
    if not ok:
        unreal.log_error(f"CONSOLIDATE_FAILED: {old_path}")
        fail_count += 1
        continue
    # rename the surviving "_Retargeted1" asset back to the base "_Retargeted" name
    rename_data = unreal.AssetRenameData(asset=new_asset, new_name=base_name, new_package_path=folder)
    renamed = asset_tools.rename_assets([rename_data])
    if renamed:
        ok_count += 1
    else:
        unreal.log_error(f"RENAME_FAILED: {new_path} -> {base_name}")
        fail_count += 1

unreal.log(f"OK: {ok_count}  FAIL: {fail_count}")
unreal.log("Saving all dirty packages...")
unreal.EditorAssetLibrary.save_directory(ROOT_PATH, only_if_is_dirty=True, recursive=True)
unreal.log("DEDUPE_DONE")
