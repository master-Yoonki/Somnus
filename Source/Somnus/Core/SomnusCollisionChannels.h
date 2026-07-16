#pragma once

#include "Engine/EngineTypes.h"

// Aliases for custom collision channels.
// Must stay in sync with Config/DefaultEngine.ini (Project Settings -> Engine -> Collision).
// If a channel is added or removed there, the ECC_GameTraceChannel# slot
// assignments can shift - update the aliases here, nowhere else.

// Weapon damage traces (melee sweeps, future hitscan).
// Channel default: Block. Character capsules ignore it; character meshes block it
// (see the CharacterMesh collision preset).
#define Somnus_TraceChannel_Weapon ECC_GameTraceChannel1
