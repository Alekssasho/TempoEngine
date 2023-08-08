#pragma once

#include <EngineCore.h>
#include <World/GameplayFeature.h>
#include <World/TaskGraph/Tasks.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace GameplayFeatures
{
struct InputController : public GameplayFeature
{
	virtual void PrepareSystems(class World& world) override
	{
		world.m_EntityWorld.system<Components::CameraController>("CameraControllerSystem")
			.kind(flecs::PreUpdate)
            .each([](flecs::entity, Components::CameraController& cameraController) {
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
			});

        world.m_EntityWorld.system<const Components::VehicleController>("VehicleControllerSystem")
            .kind(flecs::PreUpdate)
            .each([](flecs::entity, const Components::VehicleController& vehicleController) {
				auto& inputMap = *gEngine->GetInput().m_InputMaps[vehicleController.InputMapIndex].get();
				auto& physics = gEngine->GetPhysics();

				// Vehicle
				physics.VehicleInputData.setDigitalAccel(inputMap.GetBool(Accelerate));
				physics.VehicleInputData.setDigitalBrake(inputMap.GetBool(Brake));
				physics.VehicleInputData.setDigitalHandbrake(inputMap.GetBool(Handbrake));
				physics.VehicleInputData.setDigitalSteerLeft(inputMap.GetBool(SteerLeft));
				physics.VehicleInputData.setDigitalSteerRight(inputMap.GetBool(SteerRight));
             });
	}

};
}
}