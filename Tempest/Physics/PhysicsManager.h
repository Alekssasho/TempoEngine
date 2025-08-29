#pragma once

namespace Tempest
{
class World;
namespace Components
{
	struct Transform;
}

struct PhysicsImpl;
class PhysicsManager
{
public:
	PhysicsManager();
	~PhysicsManager();

	void LoadDatabase(const char* name);
	void PatchWorldComponents(World& world);
	void Update(float deltaTime);

	JPH::BodyID CreateBodyFromPrefabScene(uint32_t index, const Components::Transform& transform);
    void CopyTransformToBody(const Components::Transform& transform, JPH::BodyID id);
    void CopyTransformFromBody(Components::Transform& transform, JPH::BodyID id);
	void SetVelocity(JPH::BodyID id, glm::vec3 linear, glm::vec3 angular);
	void RemoveBody(JPH::BodyID id);

private:
	eastl::unique_ptr<PhysicsImpl> m_Impl;
};
}