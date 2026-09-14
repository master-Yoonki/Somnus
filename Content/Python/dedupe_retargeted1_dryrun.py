import unreal

ROOT_PATH = "/Game/Characters"
registry = unreal.AssetRegistryHelpers.get_asset_registry()
all_assets = registry.get_assets_by_path(ROOT_PATH, recursive=True)

dupes = [a for a in all_assets if str(a.asset_name).endswith("_Retargeted1")]
unreal.log(f"FOUND_RETARGETED1: {len(dupes)}")

pairs = []
no_counterpart = []
for d in dupes:
    dname = str(d.asset_name)
    base_name = dname[:-1]  # strip trailing "1"
    counterpart_path = f"{d.package_path}/{base_name}.{base_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(counterpart_path):
        pairs.append((counterpart_path, f"{d.package_name}.{dname}", base_name))
    else:
        no_counterpart.append(f"{d.package_name}.{dname}")

unreal.log(f"PAIRS_TO_FIX: {len(pairs)}")
unreal.log(f"NO_COUNTERPART: {len(no_counterpart)}")
for p in no_counterpart[:20]:
    unreal.log(f"  ORPHAN_1: {p}")
for old_path, new_path, base_name in pairs[:10]:
    unreal.log(f"  PAIR: {old_path} <- {new_path} (rename to {base_name})")
