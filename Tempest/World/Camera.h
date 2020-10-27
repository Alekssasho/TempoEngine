#pragma once

#include <Math/Math.h>

namespace Tempest
{
class Camera
{
public:
	void SetCamera(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up);
	void SetPerspectiveProjection(float aspectRatio, float fov, float znear, float zfar);
	glm::mat4x4 GetViewProjection() const;
private:
	glm::vec3 m_Position;
	glm::vec3 m_Target;
	glm::vec3 m_Up;
	glm::mat4x4 m_Projection;
};
}