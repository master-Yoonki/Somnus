"""Measure how fast each motion-matching clip actually travels, grouped by gait and direction.

The capsule's speed has to match the speed the animation was captured at, or motion matching
either slides the feet or reaches for the wrong clip. This reads the root motion of every clip in
the locomotion databases and prints its average speed and travel direction, so the per-direction
speed table can be filled from data instead of guessed.

Run headless:
    UnrealEditor-Win64-DebugGame-Cmd.exe Somnus.uproject -run=pythonscript -script=<this file>
Results are logged with the prefix MEASURE and also written to Saved/LocomotionSpeeds.csv.
"""
import math
import os
import unreal

DATABASE_ROOT = "/Game/Animation/Player/MotionMatching/PSDs"
# Loops are the only clips whose average speed means anything; starts, stops and pivots spend
# most of their length accelerating, so they are reported but kept apart.
SAMPLE_STEP = 1.0 / 30.0


def log(message):
    unreal.log_warning("MEASURE " + message)


def database_sequences(database):
    """Every animation sequence a pose search database draws from.

    The database keeps its asset list private in 5.7, so this goes through the asset registry's
    hard references instead - the same clips, without depending on the struct's layout.
    """
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=False,
                                                    include_hard_package_references=True,
                                                    include_searchable_names=False,
                                                    include_soft_management_references=False,
                                                    include_hard_management_references=False)
    sequences = []
    for package in registry.get_dependencies(database.get_outermost().get_name(), options) or []:
        asset = unreal.load_asset(str(package))
        if isinstance(asset, unreal.AnimSequence):
            sequences.append(asset)
    return sequences


def root_transform(sequence, time):
    return unreal.AnimationLibrary.get_bone_pose_for_time(sequence, "root", time, False)


def measure(sequence):
    """Average horizontal speed and travel direction in the clip's own starting frame.

    Direction is measured against the root's facing at the start of the clip: 0 is forward,
    +90 left, -90 right, 180 backward.
    """
    length = unreal.AnimationLibrary.get_sequence_length(sequence)
    if length <= 0.0:
        return None

    start = root_transform(sequence, 0.0)
    end = root_transform(sequence, length)
    delta = end.translation - start.translation
    distance = math.hypot(delta.x, delta.y)

    # Path length as well as displacement: a curved clip covers more ground than its endpoints say.
    path = 0.0
    previous = start.translation
    time = SAMPLE_STEP
    while time < length:
        current = root_transform(sequence, time).translation
        path += math.hypot(current.x - previous.x, current.y - previous.y)
        previous = current
        time += SAMPLE_STEP
    path += math.hypot(end.translation.x - previous.x, end.translation.y - previous.y)

    # The mannequin faces +Y in its own space; unrotate the displacement into the start facing.
    local = start.rotation.unrotate_vector(delta)
    direction = math.degrees(math.atan2(local.x, local.y)) if distance > 1.0 else 0.0

    return {
        "length": length,
        "displacement_speed": distance / length,
        "path_speed": path / length,
        "direction": direction,
    }


def main():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    databases = registry.get_assets_by_path(DATABASE_ROOT, recursive=True)

    rows = []
    for data in databases:
        if str(data.asset_class_path.asset_name) != "PoseSearchDatabase":
            continue
        database = data.get_asset()
        for sequence in database_sequences(database):
            result = measure(sequence)
            if not result:
                continue
            row = (database.get_name(), sequence.get_name(), result["length"],
                   result["displacement_speed"], result["path_speed"], result["direction"])
            rows.append(row)
            log("%-28s %-50s len=%5.2f disp=%6.1f path=%6.1f dir=%7.1f" % row)

    output = os.path.join(unreal.Paths.project_saved_dir(), "LocomotionSpeeds.csv")
    with open(output, "w") as handle:
        handle.write("database,sequence,length,displacement_speed,path_speed,direction_deg\n")
        for row in rows:
            handle.write("%s,%s,%.3f,%.1f,%.1f,%.1f\n" % row)
    log("wrote %d rows to %s" % (len(rows), output))


main()
