import unreal
registry = unreal.AssetRegistryHelpers.get_asset_registry()
psds = registry.get_assets_by_path("/Game/Characters/MotionMatching/PSDs", recursive=True)
psds = [a for a in psds if str(a.asset_class_path.asset_name) == "PoseSearchDatabase"]
for pa in psds:
    db = pa.get_asset()
    n = db.get_num_animation_assets()
    none_count = sum(1 for i in range(n) if db.get_animation_asset(i) is None)
    unreal.log(f"{pa.package_name}: total={n} none={none_count}")
