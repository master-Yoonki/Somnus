import unreal
db = unreal.EditorAssetLibrary.load_asset("/Game/Characters/MotionMatching/PSDs/Walk/PSD_Walk_Pivots")
n = db.get_num_animation_assets()
unreal.log(f"TOTAL: {n}")
none_count = 0
for i in range(n):
    e = db.get_animation_asset(i)
    if e is None:
        none_count += 1
        if none_count <= 5:
            unreal.log(f"  [{i}] NONE")
    elif none_count <= 5 and i < 5:
        unreal.log(f"  [{i}] {e.get_name()}")
unreal.log(f"NONE_COUNT: {none_count} / {n}")

# also check if the actual retargeted anim sequence files exist and load fine
test_path = "/Game/Characters/UEFN_Mannequin/Animations/Walk/M_Neutral_Walk_Pivot_B_F_Lfoot_Retargeted"
exists = unreal.EditorAssetLibrary.does_asset_exist(test_path)
unreal.log(f"TEST_ANIM_EXISTS: {exists}")
if exists:
    a = unreal.EditorAssetLibrary.load_asset(test_path)
    unreal.log(f"TEST_ANIM_CLASS: {a.get_class().get_name() if a else None}")
