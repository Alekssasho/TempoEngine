#pragma once

#include <EngineCore.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace Systems
{
struct MoveSystem
{
	static void Run(flecs::entity entity, Components::Transform& transform)
	{
		transform.Position += glm::vec3{ 1.0f, 0.0f, 0.0f };
	}
};

}
}