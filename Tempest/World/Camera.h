#pragma once

namespace Tempest
{
class Camera
{
public:
	void SetPerspectiveProjection(float aspectRatio, float fov, float znear, float zfar);
	glm::mat4x4 GetViewProjection() const;

	glm::vec3 Position;
	glm::vec3 Forward;
	glm::vec3 Up;
private:
	glm::mat4x4 m_Projection;
};
}