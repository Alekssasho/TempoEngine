#include <World/Camera.h>

namespace Tempest
{
void Camera::SetPerspectiveProjection(float aspectRatio, float fov, float znear, float zfar)
{
	m_Projection = glm::perspective(fov, aspectRatio, znear, zfar);
}

glm::mat4x4 Camera::GetViewProjection() const
{
	return m_Projection * glm::lookAt(Position, Position + Forward, Up);
}
}
