#pragma once

#include <Graphics/RendererTypes.h>
#include <World/Camera.h>
#include <flecs.h>

namespace physx
{
class PxRigidBody;
}

namespace Tempest
{
namespace CastleFight
{
enum class Faction : uint32_t
{
	Red,
	Blue,
	Count
};
}
}

#include "Generated/Components.h"

namespace Tempest
{
void RegisterComponents(flecs::world& world);
}