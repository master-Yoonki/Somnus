"""Measure how far each zombie attack montage actually reaches, from its own trace windows.

For every montage with ANS_ZombieMeleeTrace windows, this follows each traced socket through its
window, root motion included, and reports how far in front of where the attack started the sweep
gets. Distances are horizontal and measured from the body's axis at the first frame. "Hits at" adds
the trace radius and the target's capsule radius: the sweep only has to touch the capsule.

Read only - nothing is modified or saved.

Run headless with the editor closed:
    UnrealEditor-Win64-DebugGame-Cmd.exe Somnus.uproject -run=pythonscript -script=<this file>
Lines are logged with the prefix REACH.
"""
import math
import unreal

MONTAGE_FOLDER = "/Game/Zombie_Anims/RetargetedAnims/Attack"
NOTIFY_CLASS = unreal.load_class(None, "/Script/Somnus.ANS_ZombieMeleeTrace")
PLAYER_BP = "/Game/Blueprints/Character/BP_SomnusCharacter"

REGISTRY = unreal.AssetRegistryHelpers.get_asset_registry()
HARD_DEPENDENCIES = unreal.AssetRegistryDependencyOptions(
    include_soft_package_references=False, include_hard_package_references=True,
    include_searchable_names=False, include_soft_management_references=False,
    include_hard_management_references=False)


def log(message):
    unreal.log_warning("REACH " + message)


def source_sequence(montage):
    for package in REGISTRY.get_dependencies(montage.get_outermost().get_name(), HARD_DEPENDENCIES) or []:
        asset = unreal.load_asset(str(package))
        if isinstance(asset, unreal.AnimSequence):
            return asset
    return None


def player_capsule_radius():
    bp = unreal.load_asset(PLAYER_BP)
    if not bp:
        return None
    cdo = unreal.get_default_object(bp.generated_class())
    capsule = cdo.get_editor_property("capsule_component")
    return capsule.get_unscaled_capsule_radius() if capsule else None


SOCKET_CACHE = {}


def find_socket(sequence, socket_name):
    """(bone, relative transform) of a socket, looked up on the skeleton, then on meshes using it."""
    skeleton = sequence.get_editor_property("skeleton")
    key = (skeleton.get_path_name(), str(socket_name))
    if key in SOCKET_CACHE:
        return SOCKET_CACHE[key]

    found = None
    try:
        for socket in skeleton.get_editor_property("sockets") or []:
            if str(socket.get_editor_property("socket_name")) == str(socket_name):
                found = socket
                break
    except Exception:
        pass

    if not found:
        meshes = REGISTRY.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "SkeletalMesh"))
        for data in meshes:
            mesh = data.get_asset()
            if mesh and mesh.get_editor_property("skeleton") == skeleton:
                socket = mesh.find_socket(socket_name)
                if socket:
                    found = socket
                    break

    result = None
    if found:
        relative = unreal.Transform(found.get_editor_property("relative_location"),
                                    found.get_editor_property("relative_rotation"),
                                    found.get_editor_property("relative_scale"))
        result = (str(found.get_editor_property("bone_name")), relative)
    SOCKET_CACHE[key] = result
    return result


def bone_transform(sequence, frame, bone):
    """The bone's transform in the clip's space, composed down from the root."""
    path = [str(name) for name in unreal.AnimationLibrary.find_bone_path_to_root(sequence, bone)]
    transform = unreal.Transform()
    for name in reversed(path):
        transform = unreal.AnimationLibrary.get_bone_pose_for_frame(sequence, name, frame, False) * transform
    return transform


def measure_window(sequence, rate, start, end, socket_name, forward):
    """(furthest forward, furthest any direction, time of furthest forward) for one traced socket."""
    socket = find_socket(sequence, socket_name)
    if socket:
        bone, relative = socket
    else:
        bone, relative = socket_name, unreal.Transform()

    origin = bone_transform(sequence, 0, "root").translation
    best_forward, best_any, best_time = -1e9, 0.0, start
    key_count = unreal.AnimationLibrary.get_num_keys(sequence)
    first = max(0, int(math.floor(start * rate)))
    last = min(key_count - 1, int(math.ceil(end * rate)))
    for frame in range(first, last + 1):
        position = (relative * bone_transform(sequence, frame, bone)).translation
        dx, dy = position.x - origin.x, position.y - origin.y
        along = dx * forward[0] + dy * forward[1]
        if along > best_forward:
            best_forward, best_time = along, frame / rate
        best_any = max(best_any, math.hypot(dx, dy))
    return best_forward, best_any, best_time, socket is not None


def measure(montage, capsule):
    windows = []
    for event in unreal.AnimationLibrary.get_animation_notify_events(montage):
        state = event.get_editor_property("notify_state_class")
        if state and state.get_class() == NOTIFY_CLASS:
            start = unreal.AnimationLibrary.get_anim_notify_event_trigger_time(event)
            windows.append((start, start + unreal.AnimationLibrary.get_anim_notify_event_duration(event),
                            state.get_editor_property("trace_configs")))
    if not windows:
        log("%-30s no trace windows" % montage.get_name())
        return

    sequence = source_sequence(montage)
    if not sequence:
        log("%-30s no source sequence" % montage.get_name())
        return

    key_count = unreal.AnimationLibrary.get_num_keys(sequence)
    rate = (key_count - 1) / unreal.AnimationLibrary.get_sequence_length(sequence)
    root_start = bone_transform(sequence, 0, "root").translation
    root_end = bone_transform(sequence, key_count - 1, "root").translation
    travel = math.hypot(root_end.x - root_start.x, root_end.y - root_start.y)

    # The mannequin faces +Y in its own space.
    forward = (0.0, 1.0)

    for start, end, configs in windows:
        for config in configs:
            socket_name = config.get_editor_property("socket_name")
            radius = config.get_editor_property("race_radius")
            ahead, anywhere, at, socket_found = measure_window(sequence, rate, start, end, socket_name, forward)
            log("%-30s %-11s window %.2f-%.2fs  forward %5.0f  any %5.0f  at %.2fs  hits at %5.0f%s"
                % (montage.get_name(), socket_name, start, end, ahead, anywhere, at,
                   ahead + radius + capsule, "" if socket_found else "  (bone, socket not found)"))
    log("%-30s root travel %.0f  (start %.0f,%.0f  end %.0f,%.0f)"
        % (montage.get_name(), travel, root_start.x, root_start.y, root_end.x, root_end.y))


capsule = player_capsule_radius()
log("player capsule radius %s" % capsule)
capsule = capsule or 34.0

for data in REGISTRY.get_assets_by_path(MONTAGE_FOLDER, recursive=False):
    asset = data.get_asset()
    if isinstance(asset, unreal.AnimMontage):
        measure(asset, capsule)
