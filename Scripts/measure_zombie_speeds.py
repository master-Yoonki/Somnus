"""
Measure the authored forward travel speed of the zombie locomotion anims (UE 5.7).

speed = (net horizontal displacement of the moving bone) / (clip length)

We sample the pose at the first and last frame via unreal.AnimationLibrary
(AnimPose API) and measure how far 'root' and 'pelvis' translate. Whichever bone
carries the travel (root if root-motion was baked, otherwise pelvis) gives the
authored speed. Feed the Walk / Run / Sprint numbers into:
    - ASomnusZombieAIController::StateConfigs defaults
    - BS_Zombie_Locomotion sample X positions

How to run: see the block comment at the bottom of this file.
"""

import unreal

FOLDER = "/Game/Zombie_Anims/RetargetedAnims"
CANDIDATE_BONES = ["root", "pelvis"]


def _horiz(a, b, length):
    return unreal.Vector(b.x - a.x, b.y - a.y, 0.0).length() / length


def measure(seq):
    """Return dict of {method: horizontal_speed} plus clip length."""
    lib = unreal.AnimationLibrary
    length = seq.get_play_length()
    if length <= 0.0:
        return {}, length
    num_frames = lib.get_num_frames(seq)

    out = {}

    # Method 1: root motion track directly (the authoritative root-motion source)
    try:
        t0 = lib.extract_root_track_transform(seq, 0.0, [])
        t1 = lib.extract_root_track_transform(seq, length, [])
        out["root_track"] = _horiz(t0.translation, t1.translation, length)
    except Exception as exc:
        unreal.log_warning("root_track failed for {}: {}".format(seq.get_name(), exc))

    # Method 2: per-bone pose with root motion applied (cross-check; catches
    # clips where travel sits on pelvis rather than the root bone)
    for bone in CANDIDATE_BONES:
        try:
            b0 = lib.get_bone_pose_for_frame(seq, bone, 0, True)
            b1 = lib.get_bone_pose_for_frame(seq, bone, num_frames, True)
            out[bone] = _horiz(b0.translation, b1.translation, length)
        except Exception:
            pass  # bone not present / signature mismatch

    return out, length


def dump_api_once():
    """If everything fails, print the relevant method names to fix the script."""
    unreal.log("---- AnimationLibrary methods (pose/frame/bone) ----")
    for m in sorted(dir(unreal.AnimationLibrary)):
        if any(k in m.lower() for k in ("pose", "frame", "bone", "root", "num")):
            unreal.log("  AnimationLibrary." + m)


def main():
    paths = unreal.EditorAssetLibrary.list_assets(FOLDER, recursive=True, include_folder=False)

    rows = []
    any_success = False
    for path in paths:
        name = path.split("/")[-1].split(".")[0]
        if "Forward" not in name or "InPlace" in name:
            continue
        seq = unreal.load_asset(path)
        if not isinstance(seq, unreal.AnimSequence):
            continue
        try:
            speeds, length = measure(seq)
        except Exception as exc:
            unreal.log_error("measure failed for {}: {}".format(name, exc))
            continue
        if speeds:
            any_success = True
        rows.append((name, speeds, length))

    def sortkey(r):
        s = r[1]
        return max(s.values()) if s else 0.0
    rows.sort(key=sortkey)

    unreal.log("==== Zombie forward travel speeds (slow -> fast) ====")
    unreal.log("{:44s} {:>11s} {:>8s} {:>8s} {:>7s}".format(
        "clip", "root_track", "root", "pelvis", "len s"))
    for name, speeds, length in rows:
        unreal.log("{:44s} {:>11.1f} {:>8.1f} {:>8.1f} {:>7.2f}".format(
            name,
            speeds.get("root_track", 0.0),
            speeds.get("root", 0.0),
            speeds.get("pelvis", 0.0),
            length))

    if not any_success:
        unreal.log_warning("No bone speeds computed - dumping API to fix the script:")
        dump_api_once()


if __name__ == "__main__":
    main()

# ---------------------------------------------------------------------------
# RUNNING THIS SCRIPT (Unreal Editor)
#
# Prerequisite: enable the "Python Editor Script Plugin"
#   Edit > Plugins > search "Python" > tick "Python Editor Script Plugin" > restart.
#
# Option A - Output Log console (quickest):
#   1. Window > Output Log
#   2. Bottom-left dropdown: switch "Cmd" -> "Python"
#   3. Paste:  py "C:/Dev/Unreal Projects/Somnus/Scripts/measure_zombie_speeds.py"
#
# Option B - menu:  Tools > Execute Python Script... > browse to this file.
#
# Results print to the Output Log (filter by "LogPython").
# ---------------------------------------------------------------------------
