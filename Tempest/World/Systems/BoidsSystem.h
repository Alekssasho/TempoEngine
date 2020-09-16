#pragma once
#pragma once

#include <EngineCore.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace Systems
{
struct BoidsSystem : public System
{
	virtual void PrepareQueries(class World& world) override;

	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) override;
};

}
}