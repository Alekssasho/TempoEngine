#pragma once

#include <World/Camera.h>
#include <World/Navigation.h>
#include <Graphics/RendererTypes.h>
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

enum class AnimationState : uint32_t
{
	Idle,
	Move,
	Attack,
	Count
};
}

#include "Generated/Components.h"

namespace Tempest
{
void RegisterComponents(flecs::world& world);
}