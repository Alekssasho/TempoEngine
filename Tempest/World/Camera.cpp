#include <World/Camera.h>

namespace Tempest
{
void Camera::SetCamera(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
{
	m_Position = position;
	m_Target = target;
	m_Up = up;
}

void Camera::SetPerspectiveProjection(float aspectRatio, float fov, float znear, float zfar)
{
	m_Projection = glm::perspective(fov, aspectRatio, znear, zfar);
}

glm::mat4x4 Camera::GetViewProjection() const
{
	return m_Projection * glm::lookAt(m_Position, m_Target, m_Up);
}
}
