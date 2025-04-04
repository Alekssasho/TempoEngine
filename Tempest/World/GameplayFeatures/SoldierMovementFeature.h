#pragma once

#include <EngineCore.h>
#include <World/GameplayFeature.h>
#include <World/TaskGraph/Tasks.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace GameplayFeatures
{
struct SoldierMovementController : public GameplayFeature
{
	virtual void PrepareSystems(class World& world) override
	{
		world.m_EntityWorld.observer("Init Movement Controller Observer")
			.event(flecs::OnAdd)
			.with<Components::LaneMovement>().filter()
			.with<Components::Transform>().filter()
			.with<Tags::Attacks>().second(flecs::Wildcard)
			.each([](flecs::iter itr, size_t row) {
				auto e = itr.entity(row);
				auto target = e.target<Tags::Attacks>();
				auto targetTransform = target.get_ref<Components::Transform>();

				auto navData = itr.world().get<Components::NavigationData>();

				Components::LaneMovement& movement = itr.field_at<Components::LaneMovement>(0, row);

				movement.Itr = Navigation::FindClosestLane(*navData, itr.field_at<Components::Transform>(1, row).Position, targetTransform->Position);
			});


		world.m_EntityWorld.system<const Components::Transform, Components::Movement, Components::LaneMovement>("SoldierMovementController")
			.kind(flecs::PreUpdate)
			.with<Tags::SimpleMovement>()
			.each([](flecs::iter itr, size_t, const Components::Transform& transform, Components::Movement& movement, Components::LaneMovement& laneMovement) {
				if (!laneMovement.Itr.IsValid())
				{
					movement.Velocity = glm::vec3(0.0f, 0.0f, 0.0f);
					return;
				}

				auto navData = itr.world().get<Components::NavigationData>();

				movement.Velocity = laneMovement.Itr.UpdateNextDirection(*navData, transform.Position);
			});


		world.m_EntityWorld.system<Components::Transform, const Components::Movement, const Components::MovementInfo>("MovementSystem")
			.kind(flecs::OnUpdate)
			.with<Tags::SimpleMovement>()
			.each([](flecs::iter it, size_t row, Components::Transform& transform, const Components::Movement& movement, const Components::MovementInfo& movementInfo) {
				auto deltaTime = it.delta_time();

				transform.Position += movement.Velocity * movementInfo.Speed * deltaTime;
			});
	}

};
}
}