import unreal
registry = unreal.AssetRegistryHelpers.get_asset_registry()

all_assets = registry.get_assets_by_path("/Game/Characters", recursive=True)
remaining_1 = [a for a in all_assets if str(a.asset_name).endswith("_Retargeted1")]
unreal.log(f"REMAINING_RETARGETED1: {len(remaining_1)}")

psds = [a for a in all_assets if str(a.asset_class_path.asset_name) == "PoseSearchDatabase"]
total_none = 0
for pa in psds:
    db = pa.get_asset()
    n = db.get_num_animation_assets()
    none_count = sum(1 for i in range(n) if db.get_animation_asset(i) is None)
    total_none += none_count
    unreal.log(f"{pa.package_name}: total={n} none={none_count}")
unreal.log(f"TOTAL_NONE_ACROSS_ALL_PSDS: {total_none}")
