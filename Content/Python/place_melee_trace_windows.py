"""Lay a first pass of melee trace windows onto attack montages, from where the hands actually move.

For each montage with no trace yet, this follows each hand through the clip and finds the run of
frames around its fastest point - the swing - then places one trace notify per striking hand over
that run. A hand counts as striking when its peak is at least STRIKE_SHARE of the fastest hand in
the clip, which drops the idle hand and stray twitches. The windows are a starting point to check by
eye in the montage editor, not a final answer.

Run headless with the editor closed:
    UnrealEditor-Win64-DebugGame-Cmd.exe Somnus.uproject -run=pythonscript -script=<this file>
DRY_RUN prints the windows without saving. Lines are logged with the prefix TRACE.
"""
import unreal

DRY_RUN = True

MONTAGE_FOLDER = "/Game/Zombie_Anims/RetargetedAnims/Attack"
NOTIFY_CLASS = unreal.load_class(None, "/Script/Somnus.ANS_ZombieMeleeTrace")
NOTIFY_TRACK = "MeleeTrace"

# Hand bone to follow, and the socket the trace sweeps from - as set on the hand-made windows.
HANDS = {"hand_l": "HandGrip_L", "hand_r": "HandGrip_R"}
TRACE_RADIUS = 10.0

SWING_SHARE = 0.6    # frames at or above this share of a hand's peak belong to its swing
STRIKE_SHARE = 0.7   # hands whose peak reaches this share of the clip's fastest hand strike
PAD_FRAMES = 1       # widen each window so a fast swing is not missed between frames

REGISTRY = unreal.AssetRegistryHelpers.get_asset_registry()
HARD_DEPENDENCIES = unreal.AssetRegistryDependencyOptions(
    include_soft_package_references=False, include_hard_package_references=True,
    include_searchable_names=False, include_soft_management_references=False,
    include_hard_management_references=False)


def log(message):
    unreal.log_warning("TRACE " + message)


def source_sequence(montage):
    for package in REGISTRY.get_dependencies(montage.get_outermost().get_name(), HARD_DEPENDENCIES) or []:
        asset = unreal.load_asset(str(package))
        if isinstance(asset, unreal.AnimSequence):
            return asset
    return None


def has_trace(montage):
    for event in unreal.AnimationLibrary.get_animation_notify_events(montage):
        state = event.get_editor_property("notify_state_class")
        if state and state.get_class() == NOTIFY_CLASS:
            return True
    return False


def bone_position(sequence, frame, bone):
    """The bone's position in the clip's space, composed down from the root."""
    path = [str(name) for name in unreal.AnimationLibrary.find_bone_path_to_root(sequence, bone)]
    transform = unreal.Transform()
    for name in reversed(path):
        transform = unreal.AnimationLibrary.get_bone_pose_for_frame(sequence, name, frame, False) * transform
    return transform.translation


def swing(sequence, bone):
    """(peak speed, first frame, last frame) of the bone's fastest run."""
    key_count = unreal.AnimationLibrary.get_num_keys(sequence)
    rate = (key_count - 1) / unreal.AnimationLibrary.get_sequence_length(sequence)
    speeds = []
    previous = bone_position(sequence, 0, bone)
    for frame in range(1, key_count):
        current = bone_position(sequence, frame, bone)
        speeds.append((current - previous).length() * rate)
        previous = current

    peak = max(speeds)
    peak_frame = speeds.index(peak) + 1
    first = peak_frame
    while first > 1 and speeds[first - 2] >= SWING_SHARE * peak:
        first -= 1
    last = peak_frame
    while last < key_count - 1 and speeds[last] >= SWING_SHARE * peak:
        last += 1
    return peak, max(first - PAD_FRAMES, 0), min(last + PAD_FRAMES, key_count - 1), rate


def place(montage):
    if has_trace(montage):
        log("%-28s already has trace windows, left alone" % montage.get_name())
        return

    sequence = source_sequence(montage)
    if not sequence:
        log("%-28s no source sequence found" % montage.get_name())
        return

    swings = {bone: swing(sequence, bone) for bone in HANDS}
    fastest = max(peak for peak, _, _, _ in swings.values())

    windows = []
    for bone, (peak, first, last, rate) in swings.items():
        if peak >= STRIKE_SHARE * fastest:
            windows.append((HANDS[bone], first / rate, (last - first) / rate))

    log("%-28s %s" % (montage.get_name(), ", ".join(
        "%s %.2f-%.2fs" % (socket, start, start + duration) for socket, start, duration in windows)))

    if DRY_RUN:
        return

    montage.modify()
    if NOTIFY_TRACK not in [str(name) for name in unreal.AnimationLibrary.get_animation_notify_track_names(montage)]:
        unreal.AnimationLibrary.add_animation_notify_track(montage, NOTIFY_TRACK)

    for socket, start, duration in windows:
        notify = unreal.AnimationLibrary.add_animation_notify_state_event(
            montage, NOTIFY_TRACK, start, duration, NOTIFY_CLASS)
        config = unreal.SomnusZombieTraceConfig()
        config.set_editor_property("socket_name", socket)
        config.set_editor_property("race_radius", TRACE_RADIUS)
        notify.set_editor_property("trace_configs", [config])

    unreal.EditorAssetLibrary.save_loaded_asset(montage, False)
    log("  saved")


for data in REGISTRY.get_assets_by_path(MONTAGE_FOLDER, recursive=False):
    asset = data.get_asset()
    if isinstance(asset, unreal.AnimMontage):
        place(asset)
