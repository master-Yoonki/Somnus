import unreal
path = "/Game/Characters/UEFN_Mannequin/Animations/Idle/M_Neutral_Stand_Idle_Loop_Retargeted"
exists = unreal.EditorAssetLibrary.does_asset_exist(path)
unreal.log(f"EXISTS: {exists}")
if exists:
    data = unreal.EditorAssetLibrary.find_asset_data(path)
    unreal.log(f"CLASS: {data.asset_class_path}")
