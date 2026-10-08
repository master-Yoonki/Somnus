"""Move the travel in an animation from its pelvis to its root, so root motion can carry it.

Many imported clips (Mixamo-style) leave the root still and travel on the pelvis. Root motion only
reads the root, so the capsule stays put while the mesh walks away, and snaps back when the montage
ends. Retargeting with "Generate From Target Pelvis" fixes the travel but rebuilds the root's yaw
from the pelvis, which wobbles. This keeps the root's own keys - rotation included, so a turning
attack still turns - and only adds the pelvis's horizontal travel to the root's position, taking the
same amount back out of every direct child of the root so nothing under it moves in the world.

Run headless with the editor closed:
    UnrealEditor-Win64-DebugGame-Cmd.exe Somnus.uproject -run=pythonscript -script=<this file>
DRY_RUN prints what would change without saving. Lines are logged with the prefix BAKE.
"""
import unreal

DRY_RUN = True

SEQUENCES = [
    "/Game/Zombie_Anims/RetargetedAnims/Attack/Zombie_Attack_01",
    "/Game/Zombie_Anims/RetargetedAnims/Attack/Zombie_Attack_02",
    "/Game/Zombie_Anims/RetargetedAnims/Attack/Zombie_Attack_03",
    "/Game/Zombie_Anims/RetargetedAnims/Attack/Zombie_Attack_90TurnLeft",
    "/Game/Zombie_Anims/RetargetedAnims/Attack/Zombie_Attack_90TurnRight",
    "/Game/Zombie_Anims/RetargetedAnims/Attack/Zombie_Attack_180Turn",
]

ROOT = "root"
PELVIS = "pelvis"


def log(message):
    unreal.log_warning("BAKE " + message)


def flat(v):
    return unreal.Vector(v.x, v.y, 0.0)


def read_track(seq, bone, key_count):
    """The bone's local transform at every key. Read by evaluating each frame rather than through
    the raw-track getters, which return nothing for clips stored in the 5.x data model."""
    return [unreal.AnimationLibrary.get_bone_pose_for_frame(seq, bone, i, False) for i in range(key_count)]


def direct_children_of_root(seq, track_names):
    children = []
    for name in track_names:
        if name == ROOT:
            continue
        path = list(unreal.AnimationLibrary.find_bone_path_to_root(seq, name))
        # The path runs from the bone up to the root, so a direct child is the bone and the root.
        if len(path) == 2 and str(path[-1]) == ROOT:
            children.append(name)
    return children


def bake(path):
    seq = unreal.load_asset(path)
    if not seq:
        log("missing " + path)
        return

    track_names = [str(n) for n in unreal.AnimationLibrary.get_animation_track_names(seq)]
    if ROOT not in track_names or PELVIS not in track_names:
        log("%s has no %s or %s track, skipped" % (seq.get_name(), ROOT, PELVIS))
        return

    key_count = unreal.AnimationLibrary.get_num_keys(seq)
    root = read_track(seq, ROOT, key_count)
    root_pos = [t.translation for t in root]
    root_rot = [t.rotation for t in root]
    root_scale = [t.scale3d for t in root]
    pelvis_pos = [t.translation for t in read_track(seq, PELVIS, key_count)]

    # Where the pelvis is in the clip's own space at each key, through the root as it is now.
    pelvis_world = [root_rot[i].rotate_vector(pelvis_pos[i]) + root_pos[i] for i in range(key_count)]
    start = pelvis_world[0]

    # The root follows the pelvis across the floor from where it started; its height and its keys'
    # rotation are left exactly as they were.
    new_root_pos = []
    for i in range(key_count):
        travel = flat(pelvis_world[i] - start)
        new_root_pos.append(unreal.Vector(root_pos[0].x + travel.x, root_pos[0].y + travel.y, root_pos[i].z))

    # Every child keeps its place in the clip's space: only its position under the root changes, by
    # exactly what the root moved. Read in full here, before anything is written back.
    children = direct_children_of_root(seq, track_names)
    new_children = {}
    for child in children:
        child_track = read_track(seq, child, key_count)
        rebased = []
        for i in range(key_count):
            world = root_rot[i].rotate_vector(child_track[i].translation) + root_pos[i]
            rebased.append(root_rot[i].unrotate_vector(world - new_root_pos[i]))
        new_children[child] = (rebased, [t.rotation for t in child_track], [t.scale3d for t in child_track])

    last = key_count - 1
    log("%-26s keys=%d children=%s" % (seq.get_name(), key_count, children))
    log("  root  end before=(%.0f,%.0f,%.0f) after=(%.0f,%.0f,%.0f)" % (
        root_pos[last].x, root_pos[last].y, root_pos[last].z,
        new_root_pos[last].x, new_root_pos[last].y, new_root_pos[last].z))
    if PELVIS in new_children:
        p0 = new_children[PELVIS][0][0]
        pl = new_children[PELVIS][0][last]
        log("  pelvis local start=(%.0f,%.0f,%.0f) end=(%.0f,%.0f,%.0f)" % (p0.x, p0.y, p0.z, pl.x, pl.y, pl.z))

    if DRY_RUN:
        return

    controller = seq.get_editor_property("controller")
    controller.open_bracket("Bake pelvis travel into root", True)
    controller.set_bone_track_keys(ROOT, new_root_pos, root_rot, root_scale, True)
    for child, (positions, rotations, scales) in new_children.items():
        controller.set_bone_track_keys(child, positions, rotations, scales, True)
    controller.close_bracket(True)

    seq.set_editor_property("enable_root_motion", True)
    unreal.EditorAssetLibrary.save_loaded_asset(seq, False)
    log("  saved")


for sequence_path in SEQUENCES:
    bake(sequence_path)
