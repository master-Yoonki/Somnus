"""Lay a motion warping window onto each lunging zombie attack, and work out its contact distance.

A lunge is a montage whose root travels forward. Its window runs from where the root starts moving
to the moment the strike lands - the frame its furthest-reaching traced socket is furthest ahead -
so the warp has the body where it should be when the hand arrives. The root motion after the hit is
left as authored.

Contact distance is how far short of the target's centre the body should stand at that moment: the
hand's lead over the root, plus the trace radius and the target's capsule radius, less a little so
the sweep overlaps rather than grazes. Copy it into the montage's WarpContactDistance on the attack
ability; scripting cannot write that protected array.

Run headless with the editor closed:
    UnrealEditor-Win64-DebugGame-Cmd.exe Somnus.uproject -run=pythonscript -script=<this file>
DRY_RUN prints the windows without saving. Lines are logged with the prefix WARP.
"""
import math
import unreal

DRY_RUN = True

MONTAGE_FOLDER = "/Game/Zombie_Anims/RetargetedAnims/Attack"
TRACE_CLASS = unreal.load_class(None, "/Script/Somnus.ANS_ZombieMeleeTrace")
WARP_CLASS = unreal.AnimNotifyState_MotionWarping.static_class()
NOTIFY_TRACK = "MotionWarping"
WARP_TARGET = "AttackTarget"

LUNGE_MIN_TRAVEL = 50.0   # forward root travel that makes a montage a lunge
MOVING_SHARE = 0.1        # root speed at or above this share of its peak counts as moving
TARGET_CAPSULE = 34.0     # player capsule radius
OVERLAP = 10.0            # how far the sweep should sink into the capsule rather than graze it

REGISTRY = unreal.AssetRegistryHelpers.get_asset_registry()
HARD_DEPENDENCIES = unreal.AssetRegistryDependencyOptions(
    include_soft_package_references=False, include_hard_package_references=True,
    include_searchable_names=False, include_soft_management_references=False,
    include_hard_management_references=False)

# The mannequin faces +Y in its own space.
FORWARD = (0.0, 1.0)


def log(message):
    unreal.log_warning("WARP " + message)


def source_sequence(montage):
    for package in REGISTRY.get_dependencies(montage.get_outermost().get_name(), HARD_DEPENDENCIES) or []:
        asset = unreal.load_asset(str(package))
        if isinstance(asset, unreal.AnimSequence):
            return asset
    return None


def find_socket(sequence, socket_name):
    """(bone, relative transform) of a socket on the skeleton or a mesh using it."""
    skeleton = sequence.get_editor_property("skeleton")
    # Skeleton sockets are protected from scripting; the meshes built on it carry them readably.
    for data in REGISTRY.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "SkeletalMesh")):
        mesh = data.get_asset()
        if mesh and mesh.get_editor_property("skeleton") == skeleton:
            socket = mesh.find_socket(socket_name)
            if socket:
                return socket_parts(socket)
    return str(socket_name), unreal.Transform()


def socket_parts(socket):
    return (str(socket.get_editor_property("bone_name")),
            unreal.Transform(socket.get_editor_property("relative_location"),
                             socket.get_editor_property("relative_rotation"),
                             socket.get_editor_property("relative_scale")))


def bone_transform(sequence, frame, bone):
    path = [str(name) for name in unreal.AnimationLibrary.find_bone_path_to_root(sequence, bone)]
    transform = unreal.Transform()
    for name in reversed(path):
        transform = unreal.AnimationLibrary.get_bone_pose_for_frame(sequence, name, frame, False) * transform
    return transform


def ahead(position):
    return position.x * FORWARD[0] + position.y * FORWARD[1]


def notify_windows(montage, notify_class):
    windows = []
    for event in unreal.AnimationLibrary.get_animation_notify_events(montage):
        state = event.get_editor_property("notify_state_class")
        if state and state.get_class() == notify_class:
            start = unreal.AnimationLibrary.get_anim_notify_event_trigger_time(event)
            windows.append((start, start + unreal.AnimationLibrary.get_anim_notify_event_duration(event), state))
    return windows


def place(montage):
    name = montage.get_name()
    if notify_windows(montage, WARP_CLASS):
        log("%-30s already has a warp window, left alone" % name)
        return

    traces = notify_windows(montage, TRACE_CLASS)
    sequence = source_sequence(montage)
    if not traces or not sequence:
        return

    key_count = unreal.AnimationLibrary.get_num_keys(sequence)
    rate = (key_count - 1) / unreal.AnimationLibrary.get_sequence_length(sequence)
    roots = [bone_transform(sequence, frame, "root").translation for frame in range(key_count)]
    origin = roots[0]
    travel = ahead(roots[-1]) - ahead(origin)
    if travel < LUNGE_MIN_TRAVEL:
        return

    # The strike lands where the furthest-reaching traced socket is furthest ahead.
    hit_frame, hit_reach, hit_radius, hit_root = 0, -1e9, 0.0, 0.0
    for start, end, state in traces:
        for config in state.get_editor_property("trace_configs"):
            bone, relative = find_socket(sequence, config.get_editor_property("socket_name"))
            first = max(0, int(math.floor(start * rate)))
            last = min(key_count - 1, int(math.ceil(end * rate)))
            for frame in range(first, last + 1):
                reach = ahead((relative * bone_transform(sequence, frame, bone)).translation) - ahead(origin)
                if reach > hit_reach:
                    hit_frame, hit_reach = frame, reach
                    hit_radius = config.get_editor_property("race_radius")
                    hit_root = ahead(roots[frame]) - ahead(origin)

    speeds = [abs(ahead(roots[f]) - ahead(roots[f - 1])) * rate for f in range(1, hit_frame + 1)]
    peak = max(speeds) if speeds else 0.0
    move_frame = next((f for f, speed in enumerate(speeds) if speed >= MOVING_SHARE * peak), 0)

    window_start, window_end = move_frame / rate, hit_frame / rate
    contact = (hit_reach - hit_root) + hit_radius + TARGET_CAPSULE - OVERLAP
    log("%-30s window %.2f-%.2fs  root by hit %4.0f of %4.0f  WarpContactDistance %4.0f"
        % (name, window_start, window_end, hit_root, travel, contact))

    if DRY_RUN:
        return

    montage.modify()
    if NOTIFY_TRACK not in [str(n) for n in unreal.AnimationLibrary.get_animation_notify_track_names(montage)]:
        unreal.AnimationLibrary.add_animation_notify_track(montage, NOTIFY_TRACK)

    notify = unreal.AnimationLibrary.add_animation_notify_state_event(
        montage, NOTIFY_TRACK, window_start, window_end - window_start, WARP_CLASS)
    modifier = unreal.new_object(unreal.RootMotionModifier_SkewWarp, outer=notify)
    modifier.set_editor_property("warp_target_name", WARP_TARGET)
    modifier.set_editor_property("warp_translation", True)
    modifier.set_editor_property("ignore_z_axis", True)
    modifier.set_editor_property("warp_rotation", True)
    notify.set_editor_property("root_motion_modifier", modifier)

    unreal.EditorAssetLibrary.save_loaded_asset(montage, False)
    log("  saved")


for data in REGISTRY.get_assets_by_path(MONTAGE_FOLDER, recursive=False):
    asset = data.get_asset()
    if isinstance(asset, unreal.AnimMontage):
        place(asset)
