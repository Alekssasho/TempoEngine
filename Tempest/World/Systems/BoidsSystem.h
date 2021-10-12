#pragma once
#pragma once

#include <EngineCore.h>
#include <World/Components/Components.h>
#include <World/EntityQuery.h>

namespace Tempest
{
namespace Systems
{
struct BoidsSystem : public System
{
	virtual void PrepareQueries(class World& world) override;

	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) override;

	EntityQuery<Components::Transform, Tags::Boids> m_Query;
};

}
}