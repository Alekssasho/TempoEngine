#pragma once

#include <EngineCore.h>
#include <World/System.h>
#include <World/TaskGraph/Tasks.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace Systems
{
struct CameraControllerSystem : public System
{
	EntityQuery<Components::CameraController> m_Query;

	virtual void PrepareQueries(class World& world) override
	{
		// TODO: Maybe we can add transform component, and use that for positioning, instead of duplicated data in the Camera object
		m_Query.Init(world);
	}

	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) override
	{
		TaskGraph::TaskHandle handle = graph.AddTask(
			"CameraControllerSystem::ParallelEach",
			new Task::ParallelQueryEach(
				&m_Query,
				[deltaTime](flecs::entity, Components::CameraController& cameraController) {
					auto& inputMap = *gEngine->GetInput().m_InputMaps[cameraController.InputMapIndex].get();
					auto& camera = cameraController.CameraData;

					// Update camera stuff
					auto cameraForward = glm::normalize(camera.Forward);
					auto cameraRight = glm::normalize(glm::cross(camera.Up, cameraForward));
					auto speed = 0.5f;

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
		}, 0));
	}
};

struct VehicleControllerSystem : public System
{
	EntityQuery<Components::VehicleController> m_Query;

	virtual void PrepareQueries(class World& world) override
	{
		// TODO: Maybe we can add transform component, and use that for positioning, instead of duplicated data in the Camera object
		m_Query.Init(world);
	}

	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) override
	{
		TaskGraph::TaskHandle handle = graph.AddTask(
			"VehicleControllerSystem::ParallelEach",
			new Task::ParallelQueryEach(
				&m_Query,
				[deltaTime](flecs::entity, Components::VehicleController& vehicleController) {
					auto& inputMap = *gEngine->GetInput().m_InputMaps[vehicleController.InputMapIndex].get();
					auto& physics = gEngine->GetPhysics();

					// Vehicle
					physics.VehicleInputData.setDigitalAccel(inputMap.GetBool(Accelerate));
					physics.VehicleInputData.setDigitalBrake(inputMap.GetBool(Brake));
					physics.VehicleInputData.setDigitalHandbrake(inputMap.GetBool(Handbrake));
					physics.VehicleInputData.setDigitalSteerLeft(inputMap.GetBool(SteerLeft));
					physics.VehicleInputData.setDigitalSteerRight(inputMap.GetBool(SteerRight));
		}, 1));
	}
};

}
}