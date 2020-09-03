#pragma once
#pragma once

#include <EngineCore.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace Systems
{
struct BoidsSystem
{
	void Run(flecs::world& world);
};

}
}