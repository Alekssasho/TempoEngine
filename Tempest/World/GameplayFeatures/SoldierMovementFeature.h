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

				auto& navData = itr.world().get<Components::NavigationData>();

				Components::LaneMovement& movement = itr.field_at<Components::LaneMovement>(0, row);

				auto factionFlag = uint64_t(e.get<Components::Faction>().FactionFlag);

				movement.Itr = Navigation::FindLane(navData, itr.field_at<Components::Transform>(1, row).Position, targetTransform->Position, e.id() + factionFlag * 100000);
			});


		world.m_EntityWorld.system<const Components::Transform, Components::Movement, Components::LaneMovement, const Components::Attacking, const Components::AttackInfo, Components::AnimationController>("SoldierMovementController")
			.kind(flecs::PreUpdate)
			.multi_threaded()
			.with<Tags::SimpleMovement>()
			.each([](flecs::iter itr, size_t, const Components::Transform& transform, Components::Movement& movement, Components::LaneMovement& laneMovement, const Components::Attacking& att, const Components::AttackInfo& attackInfo, Components::AnimationController& animController) {
				ZoneScopedN("SoldierMovementController");
				if (laneMovement.Itr.IsValid())
				{
					auto& navData = itr.world().get<Components::NavigationData>();

					movement.Velocity = laneMovement.Itr.UpdateNextDirection(navData, transform.Position);
				}
				else
				{
					movement.Velocity = glm::vec3(0.0f, 0.0f, 0.0f);
				}

				if (att.Target.is_valid() && att.Target.is_alive())
				{

					const auto& targetPos = att.Target.get<Components::Transform>().Position;
					if (glm::distance2(transform.Position, targetPos) > attackInfo.Range * attackInfo.Range)
					{
						movement.Velocity = glm::normalize(targetPos - transform.Position);
					}
					else
					{
						movement.Velocity = glm::vec3(0.0f, 0.0f, 0.0f);
					}
				}

				if (glm::length2(movement.Velocity) == 0.0f)
				{
					if (animController.State == AnimationState::Move)
					{
						animController.State = AnimationState::Idle;
					}
				}
				else
				{
					if (animController.State == AnimationState::Idle)
					{
						animController.State = AnimationState::Move;
					}
				}
			});


		world.m_EntityWorld.system<Components::Transform, const Components::Movement, const Components::MovementInfo>("MovementSystem")
			.kind(flecs::PostUpdate)
			.multi_threaded()
			.with<Tags::SimpleMovement>()
			.each([](flecs::iter it, size_t row, Components::Transform& transform, const Components::Movement& movement, const Components::MovementInfo& movementInfo) {
				ZoneScopedN("MovementSystem");
				auto deltaTime = it.delta_time();

				transform.Position += movement.Velocity * movementInfo.Speed * deltaTime;
				if (glm::length2(movement.Velocity) != 0.0f)
				{
					transform.Rotation = glm::rotation(sForwardDirection, movement.Velocity);
				}
			});
	}

};
}
}