"""
Set every BS_ZombieLocomotion sample's Speed (Y) axis to the authored (measured)
speed of its clip family, leaving Direction (X) and the clip itself untouched.

Authored speeds come from measure_zombie_speeds.py (pelvis horizontal travel).
Keeps the user's placed clips; only fixes the tedious Y positions.

Editor MUST be closed when running this (headless commandlet writes the .uasset).
"""
import unreal

BS_PATH = "/Game/Animation/Zombie/BS_ZombieLocomotion"

# clip-identity substring -> authored speed (cm/s). Order matters: check the
# more specific tokens first so "Walk_Fast01" / "Walk_04" don't fall into "Walk_01".
SPEED_TABLE = [
    ("Idle",        0.0),
    ("Walk_Fast01", 127.0),
    ("Walk_04",      22.0),
    ("Walk_01",      38.0),
    ("Run_02",      540.0),
    ("Run_01",      320.0),
    ("Sprint",      697.0),
]


def authored_speed(name):
    for token, spd in SPEED_TABLE:
        if token in name:
            return spd
    return None


def main():
    bs = unreal.load_asset(BS_PATH)
    if not bs:
        unreal.log_error("could not load " + BS_PATH)
        return

    samples = bs.get_editor_property("sample_data")
    new_samples = []
    changed = 0
    for s in samples:
        anim = s.get_editor_property("animation")
        name = anim.get_name() if anim else ""
        spd = authored_speed(name)
        v = s.get_editor_property("sample_value")
        if spd is None:
            unreal.log_warning("UNKNOWN clip, left as-is: {} (y={:.1f})".format(name, v.y))
            new_samples.append(s)
            continue
        if abs(v.y - spd) > 0.01:
            changed += 1
        s.set_editor_property("sample_value", unreal.Vector(v.x, spd, 0.0))
        unreal.log("  {:42s} dir={:7.1f}  y {:7.1f} -> {:7.1f}".format(name, v.x, v.y, spd))
        new_samples.append(s)

    # Setting the whole array back fires PostEditChangeProperty -> grid rebuild.
    bs.set_editor_property("sample_data", new_samples)

    saved = unreal.EditorAssetLibrary.save_loaded_asset(bs)
    unreal.log("==== updated {} / {} samples, saved={} ====".format(changed, len(samples), saved))


if __name__ == "__main__":
    main()
