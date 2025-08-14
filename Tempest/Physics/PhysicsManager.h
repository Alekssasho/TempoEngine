#pragma once

namespace Tempest
{
struct PhysicsImpl;

class PhysicsManager
{
public:
	PhysicsManager();
	~PhysicsManager();

	void LoadDatabase(const char* name);
//	void PatchWorldComponents(World& world, const eastl::vector<flecs::entity_t>& newlyCreatedEntities);
//	void Update(float deltaTime);

private:
	eastl::unique_ptr<PhysicsImpl> m_Impl;
};
}