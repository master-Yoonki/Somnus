"""Copy the hand-made motion warping setup of one montage per kind onto the rest of that kind.

Each kind has a reference montage set up by hand. Its windows are carried over as proportions of
the time to the first trace, so a montage that strikes later gets a longer wind-up window; a
lunge's translation window keeps running to its own measured hit time. Any warp windows already on
a target montage are replaced.

    in place : AM_Zombie_Attack01_InPlace -> 02..05_InPlace
    lunge    : AM_Zombie_Attack_01        -> Attack_02, Attack_03
    turn     : AM_Zombie_Attack_180Turn   -> 90TurnLeft, 90TurnRight

Run headless with the editor closed:
    UnrealEditor-Win64-DebugGame-Cmd.exe Somnus.uproject -run=pythonscript -script=<this file>
DRY_RUN prints the windows without saving. Lines are logged with the prefix POPULATE.
"""
import unreal

DRY_RUN = True

FOLDER = "/Game/Zombie_Anims/RetargetedAnims/Attack/"
TRACE_CLASS = unreal.load_class(None, "/Script/Somnus.ANS_ZombieMeleeTrace")
WARP_CLASS = unreal.AnimNotifyState_MotionWarping.static_class()
TRANSLATION_TRACK = "MWTranslation"
ROTATION_TRACK = "MWRotation"
OLD_TRACKS = ["MotionWarping", "MWTranslation", "MWRotation", "MWTranlation"]

# Measured by place_attack_warp_windows.py: when each lunge's strike lands.
LUNGE_HIT_TIME = {"AM_Zombie_Attack_01": 0.600, "AM_Zombie_Attack_02": 0.633, "AM_Zombie_Attack_03": 0.633}

KINDS = [
    ("AM_Zombie_Attack01_InPlace", ["AM_Zombie_Attack02_InPlace", "AM_Zombie_Attack03_InPlace",
                                    "AM_Zombie_Attack04_InPlace", "AM_Zombie_Attack05_InPlace"]),
    ("AM_Zombie_Attack_01", ["AM_Zombie_Attack_02", "AM_Zombie_Attack_03"]),
    ("AM_Zombie_Attack_180Turn", ["AM_Zombie_Attack_90TurnLeft", "AM_Zombie_Attack_90TurnRight"]),
]

MODIFIER_PROPS = ["warp_target_name", "warp_translation", "ignore_z_axis", "warp_to_feet_location",
                  "warp_rotation", "rotation_type", "rotation_method", "warp_rotation_time_multiplier",
                  "warp_max_rotation_rate", "subtract_remaining_root_motion", "max_speed_clamp_ratio"]


def log(message):
    unreal.log_warning("POPULATE " + message)


def windows(montage, notify_class):
    found = []
    for event in unreal.AnimationLibrary.get_animation_notify_events(montage):
        state = event.get_editor_property("notify_state_class")
        if state and state.get_class() == notify_class:
            start = unreal.AnimationLibrary.get_anim_notify_event_trigger_time(event)
            found.append((start, start + unreal.AnimationLibrary.get_anim_notify_event_duration(event), state))
    return found


def first_trace(montage):
    return min(start for start, _, _ in windows(montage, TRACE_CLASS))


def reference_windows(montage):
    """Each warp window as (track, start share, end share or 'hit', modifier values)."""
    strike = first_trace(montage)
    hit = LUNGE_HIT_TIME.get(montage.get_name())
    result = []
    for start, end, state in windows(montage, WARP_CLASS):
        modifier = state.get_editor_property("root_motion_modifier")
        values = {prop: modifier.get_editor_property(prop) for prop in MODIFIER_PROPS}
        track = TRANSLATION_TRACK if values["warp_translation"] else ROTATION_TRACK
        # A lunge's translation window runs to its strike landing, wherever that falls.
        end_share = "hit" if (track == TRANSLATION_TRACK and hit and abs(end - hit) < 0.02) else end / strike
        result.append((track, start / strike, end_share, values))
    return result


def populate(reference, target):
    strike = first_trace(target)
    planned = []
    for track, start_share, end_share, values in reference_windows(reference):
        start = start_share * strike
        end = LUNGE_HIT_TIME[target.get_name()] if end_share == "hit" else end_share * strike
        planned.append((track, start, end, values))
        log("%-28s %-13s %.3f-%.3f  rate %s  speed clamp %s"
            % (target.get_name(), track, start, end, values["warp_max_rotation_rate"], values["max_speed_clamp_ratio"]))

    if DRY_RUN:
        return

    target.modify()
    existing = [str(n) for n in unreal.AnimationLibrary.get_animation_notify_track_names(target)]
    for track in OLD_TRACKS:
        if track in existing:
            unreal.AnimationLibrary.remove_animation_notify_track(target, track)

    for track in (TRANSLATION_TRACK, ROTATION_TRACK):
        if any(t == track for t, _, _, _ in planned):
            unreal.AnimationLibrary.add_animation_notify_track(target, track)

    for track, start, end, values in planned:
        notify = unreal.AnimationLibrary.add_animation_notify_state_event(target, track, start, end - start, WARP_CLASS)
        modifier = unreal.new_object(unreal.RootMotionModifier_SkewWarp, outer=notify)
        for prop, value in values.items():
            modifier.set_editor_property(prop, value)
        notify.set_editor_property("root_motion_modifier", modifier)

    unreal.EditorAssetLibrary.save_loaded_asset(target, False)
    log("%-28s saved" % target.get_name())


for reference_name, target_names in KINDS:
    reference = unreal.load_asset(FOLDER + reference_name)
    log("%s is the reference" % reference_name)
    for track, start_share, end_share, values in reference_windows(reference):
        log("  %-13s %.2f-%s of the wind-up" % (track, start_share,
                                                "hit" if end_share == "hit" else "%.2f" % end_share))
    for target_name in target_names:
        populate(reference, unreal.load_asset(FOLDER + target_name))
