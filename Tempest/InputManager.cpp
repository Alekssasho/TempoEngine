#include <CommonIncludes.h>

#include <InputManager.h>

namespace Tempest
{

// TODO: Initialize m_input without system time and with our custom allocator
InputManager::InputManager(uint32_t width, uint32_t height)
{
	m_Input.SetDisplaySize(width, height);

	// TODO: This needs to be some kind of config file
	// 0 is camera controller
	// 1 is vehicle controller
	m_InputMaps.reserve(2);
	// NB: gainput is designed in such a way that InputMap cannot be put into stl vectors/arrays due to missing
	// default constructor and no copy constructors
	m_InputMaps.emplace_back(new gainput::InputMap(m_Input));
	m_InputMaps.emplace_back(new gainput::InputMap(m_Input));

	const gainput::DeviceId keyboardId = m_Input.CreateDevice<gainput::InputDeviceKeyboard>();
	const gainput::DeviceId mouseId = m_Input.CreateDevice<gainput::InputDeviceMouse>();

	m_InputMaps[0]->MapBool(MoveForward, keyboardId, gainput::KeyW);
	m_InputMaps[0]->MapBool(MoveBackward, keyboardId, gainput::KeyS);
	m_InputMaps[0]->MapBool(MoveLeft, keyboardId, gainput::KeyA);
	m_InputMaps[0]->MapBool(MoveRight, keyboardId, gainput::KeyD);
	m_InputMaps[0]->MapBool(FasterSpeed, keyboardId, gainput::KeyShiftL);

	m_InputMaps[0]->MapBool(MoveCameraOrientation, mouseId, gainput::MouseButtonLeft);
	m_InputMaps[0]->MapFloat(MouseX, mouseId, gainput::MouseAxisX);
	m_InputMaps[0]->MapFloat(MouseY, mouseId, gainput::MouseAxisY);


	m_InputMaps[1]->MapBool(Accelerate, keyboardId, gainput::KeyUp);
	m_InputMaps[1]->MapBool(Brake, keyboardId, gainput::KeyDown);
	m_InputMaps[1]->MapBool(Handbrake, keyboardId, gainput::KeySpace);
	m_InputMaps[1]->MapBool(SteerLeft, keyboardId, gainput::KeyLeft);
	m_InputMaps[1]->MapBool(SteerRight, keyboardId, gainput::KeyRight);
}

}
