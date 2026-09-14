import unreal

ROOT_PATH = "/Game/Characters"
registry = unreal.AssetRegistryHelpers.get_asset_registry()
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

all_assets = registry.get_assets_by_path(ROOT_PATH, recursive=True)
dupes = [a for a in all_assets if str(a.asset_name).endswith("_Retargeted1")]
unreal.log(f"TO_RENAME: {len(dupes)}")

rename_datas = []
for d in dupes:
    dname = str(d.asset_name)
    base_name = dname[:-1]
    obj = unreal.EditorAssetLibrary.load_asset(f"{d.package_name}.{dname}")
    if obj is None:
        unreal.log_error(f"Failed to load {d.package_name}")
        continue
    rename_datas.append(unreal.AssetRenameData(asset=obj, new_name=base_name, new_package_path=str(d.package_path)))

unreal.log(f"PREPARED: {len(rename_datas)}")
ok = asset_tools.rename_assets(rename_datas)
unreal.log(f"RENAME_RESULT: {ok}")
unreal.EditorAssetLibrary.save_directory(ROOT_PATH, only_if_is_dirty=True, recursive=True)
unreal.log("RENAME_DONE")
