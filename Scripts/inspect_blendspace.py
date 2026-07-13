"""Inspect BS_ZombieLocomotion structure before scripting sample-position edits."""
import unreal

BS_PATH = "/Game/Animation/Zombie/BS_ZombieLocomotion"

bs = unreal.load_asset(BS_PATH)
if not bs:
    unreal.log_error("could not load " + BS_PATH)
else:
    unreal.log("==== class: {} ====".format(bs.get_class().get_name()))

    unreal.log("---- methods (sample/axis/blend) ----")
    for m in sorted(dir(bs)):
        if any(k in m.lower() for k in ("sample", "axis", "blend")) and not m.startswith("__"):
            unreal.log("  bs." + m)

    unreal.log("---- editor properties (axes) ----")
    for prop in ("horizontal_axis", "vertical_axis", "axis_to_scale_animation",
                 "blend_parameters"):
        try:
            unreal.log("  {} = {}".format(prop, bs.get_editor_property(prop)))
        except Exception as exc:
            unreal.log_warning("  no prop '{}': {}".format(prop, exc))

    unreal.log("---- sample_data ----")
    try:
        samples = bs.get_editor_property("sample_data")
        unreal.log("  count = {}".format(len(samples)))
        for i, s in enumerate(samples):
            anim = s.get_editor_property("animation")
            val = s.get_editor_property("sample_value")
            aname = anim.get_name() if anim else "None"
            unreal.log("  [{}] anim={:42s} value={}".format(i, aname, val))
    except Exception as exc:
        unreal.log_error("  sample_data read failed: " + str(exc))

    unreal.log("---- AnimationBlendSpaceSampleLibrary (if present) ----")
    lib = getattr(unreal, "AnimationBlendSpaceSampleLibrary", None)
    if lib is None:
        unreal.log("  (not available)")
    else:
        for m in sorted(dir(lib)):
            if not m.startswith("_"):
                unreal.log("  lib." + m)
