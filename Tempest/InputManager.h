#pragma once

#pragma warning(push, 0)
#include <gainput/gainput.h>
#pragma warning(pop)

namespace Tempest
{
struct InputManager
{
	gainput::InputManager m_Input;

	eastl::vector<eastl::unique_ptr<gainput::InputMap>> m_InputMaps;

	void Update(float deltaTime)
	{
		m_Input.Update();
	}

	InputManager(uint32_t width, uint32_t height);
};

// TODO: Find better place for this
enum CameraMovement
{
	MoveForward,
	MoveBackward,
	MoveLeft,
	MoveRight,
	FasterSpeed,
	MoveCameraOrientation,
	MouseX,
	MouseY,
};

enum VehicleMovement
{
	Accelerate,
	Brake,
	Handbrake,
	SteerLeft,
	SteerRight,
};
}