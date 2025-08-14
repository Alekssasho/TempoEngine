#pragma once

#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

namespace Tempest
{
namespace Physics
{
namespace ObjectLayers
{
    static constexpr JPH::ObjectLayer Static = 0;
    static constexpr JPH::ObjectLayer Dynamic = 1;
}

namespace BroadPhaseLayers
{
    static constexpr JPH::BroadPhaseLayer sStatic(0);
    static constexpr JPH::BroadPhaseLayer sDynamic(1);
}
}
}