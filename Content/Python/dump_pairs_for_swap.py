import unreal
ROOT_PATH = "/Game/Characters"
registry = unreal.AssetRegistryHelpers.get_asset_registry()
all_assets = registry.get_assets_by_path(ROOT_PATH, recursive=True)
dupes = [a for a in all_assets if str(a.asset_name).endswith("_Retargeted1")]

lines = []
for d in dupes:
    dname = str(d.asset_name)
    base_name = dname[:-1]
    old_exists = unreal.EditorAssetLibrary.does_asset_exist(f"{d.package_path}/{base_name}.{base_name}")
    if old_exists:
        rel = str(d.package_name)[len("/Game/"):]  # Characters/.../X_Retargeted1
        old_rel = rel[:-1]  # strip trailing "1" -> Characters/.../X_Retargeted
        lines.append(old_rel)

with open(r"C:\Users\mrgna\.claude\jobs\bd54000d\tmp\old_retargeted_to_delete.txt", "w") as f:
    for l in lines:
        f.write(l + "\n")
unreal.log(f"WROTE {len(lines)} lines")
