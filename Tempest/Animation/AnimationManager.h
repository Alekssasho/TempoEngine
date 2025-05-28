#pragma once

namespace Tempest
{
namespace Definition {
    struct AnimationDatabase;
}

class AnimationManager
{
public:
	AnimationManager();

	void LoadDatabase(const char* databaseName);
	void InitializeBoneTransforms(uint32_t skeletonIndex, eastl::vector<glm::mat4x4>& transforms);
private:
	const Definition::AnimationDatabase* m_Database;
};
}