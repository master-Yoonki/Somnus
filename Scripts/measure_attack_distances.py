import unreal

def main():
    lib = unreal.AnimationLibrary
    paths = unreal.EditorAssetLibrary.list_assets("/Game/Zombie_Anims/", recursive=True, include_folder=False)
    unreal.log("=== ATTACK ROOT MOTION DISTANCES ===")
    for path in paths:
        if "Attack" not in path: continue
        seq = unreal.load_asset(path)
        if not isinstance(seq, unreal.AnimSequence): continue
        try:
            t0 = lib.extract_root_track_transform(seq, 0.0, [])
            t1 = lib.extract_root_track_transform(seq, seq.get_play_length(), [])
            dist = unreal.Vector(t1.translation.x - t0.translation.x, t1.translation.y - t0.translation.y, 0.0).length()
            unreal.log("ZOMBIE_DIST: {:35s} moves {:.1f} cm".format(seq.get_name(), dist))
        except Exception as e:
            unreal.log("ZOMBIE_DIST_ERR: {} failed: {}".format(seq.get_name(), e))
    unreal.log("====================================")

if __name__ == "__main__":
    main()
