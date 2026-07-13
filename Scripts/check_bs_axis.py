"""Check (and if needed fix) BS_ZombieLocomotion Speed-axis max so 697 fits."""
import unreal

BS_PATH = "/Game/Animation/Zombie/BS_ZombieLocomotion"
NEEDED_MAX = 700.0  # highest sample speed is Sprint @ 697

bs = unreal.load_asset(BS_PATH)
params = bs.get_editor_property("blend_parameters")  # [X(dir), Y(speed), Z]

for i, p in enumerate(params):
    try:
        unreal.log("axis[{}] name='{}' min={} max={} grid={}".format(
            i,
            p.get_editor_property("display_name"),
            p.get_editor_property("min"),
            p.get_editor_property("max"),
            p.get_editor_property("grid_num")))
    except Exception as exc:
        unreal.log_warning("axis[{}] read failed: {}".format(i, exc))

# Y axis = speed = index 1
speed_axis = params[1]
cur_max = speed_axis.get_editor_property("max")
if cur_max < NEEDED_MAX:
    speed_axis.set_editor_property("max", NEEDED_MAX)
    params[1] = speed_axis
    bs.set_editor_property("blend_parameters", params)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(bs)
    unreal.log("RAISED speed-axis max {} -> {}, saved={}".format(cur_max, NEEDED_MAX, saved))
else:
    unreal.log("speed-axis max {} already >= {}, no change".format(cur_max, NEEDED_MAX))
