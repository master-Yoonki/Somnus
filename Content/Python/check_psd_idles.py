import unreal
db = unreal.EditorAssetLibrary.load_asset("/Game/Characters/MotionMatching/PSDs/PSD_Idles")
n = db.get_num_animation_assets()
unreal.log(f"TOTAL: {n}")
for i in range(n):
    e = db.get_animation_asset(i)
    unreal.log(f"  [{i}] {e}")
