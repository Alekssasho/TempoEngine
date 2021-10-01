#pragma once

#include <EngineCore.h>
#include <World/System.h>
#include <World/EntityQueryImpl.h> // For Query Init which is templated
#include <World/TaskGraph/Tasks.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace Systems
{
struct CameraControllerSystem : public System
{
	virtual void PrepareQueries(class World& world) override
	{
		// TODO: Maybe we can add transform component, and use that for positioning, instead of duplicated data in the Camera object
		m_Query.Init<Components::CameraController>(world);
	}

	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) override
	{
		TaskGraph::TaskHandle handle = graph.CreateTask<Task::ParallelQueryEach>(
			"CameraControllerSystem::ParallelEach",
			&m_Query,
			[deltaTime](uint32_t, ecs_iter_t* iter) {
			Components::CameraController* cameraControllers = ecs_column(iter, Components::CameraController, 1);
			for (int i = 0; i < iter->count; ++i)
			{
				auto& inputMap = *gEngine->GetInput().m_InputMaps[cameraControllers[i].InputMapIndex].get();
				auto& camera = cameraControllers[i].CameraData;

				// Update camera stuff
				auto cameraForward = glm::normalize(camera.Forward);
				auto cameraRight = glm::normalize(glm::cross(camera.Up, cameraForward));
				auto speed = 2.0f;

				// Speed bump
				if (inputMap.GetBool(FasterSpeed))
				{
					speed *= 4.0f;
				}

				// Change position of camera
				if (inputMap.GetBool(MoveForward))
				{
					camera.Position += cameraForward * speed;
				}
				if (inputMap.GetBool(MoveBackward))
				{
					camera.Position += -cameraForward * speed;
				}
				if (inputMap.GetBool(MoveLeft))
				{
					camera.Position += -cameraRight * speed;
				}
				if (inputMap.GetBool(MoveRight))
				{
					camera.Position += cameraRight * speed;
				}

				// Change target
				if (inputMap.GetBool(MoveCameraOrientation))
				{
					camera.Forward = glm::rotate(camera.Forward, -inputMap.GetFloatDelta(MouseX) * 5.0f, camera.Up);
					camera.Forward = glm::rotate(camera.Forward, -inputMap.GetFloatDelta(MouseY) * 5.0f, cameraRight);
					camera.Up = glm::normalize(glm::cross(camera.Forward, cameraRight));
				}
			}
		});
	}
};

struct VehicleControllerSystem : public System
{
	virtual void PrepareQueries(class World& world) override
	{
		// TODO: Maybe we can add transform component, and use that for positioning, instead of duplicated data in the Camera object
		m_Query.Init<Components::VehicleController>(world);
	}

	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) override
	{
		TaskGraph::TaskHandle handle = graph.CreateTask<Task::ParallelQueryEach>(
			"VehicleControllerSystem::ParallelEach",
			&m_Query,
			[deltaTime](uint32_t, ecs_iter_t* iter) {
			Components::VehicleController* vehicleControllers = ecs_column(iter, Components::VehicleController, 1);
			for (int i = 0; i < iter->count; ++i)
			{
				auto& inputMap = *gEngine->GetInput().m_InputMaps[vehicleControllers[i].InputMapIndex].get();
				auto& physics = gEngine->GetPhysics();

				// Vehicle
				physics.VehicleInputData.setDigitalAccel(inputMap.GetBool(Accelerate));
				physics.VehicleInputData.setDigitalBrake(inputMap.GetBool(Brake));
				physics.VehicleInputData.setDigitalHandbrake(inputMap.GetBool(Handbrake));
				physics.VehicleInputData.setDigitalSteerLeft(inputMap.GetBool(SteerLeft));
				physics.VehicleInputData.setDigitalSteerRight(inputMap.GetBool(SteerRight));
			}
		});
	}
};

}
}