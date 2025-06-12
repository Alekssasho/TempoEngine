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
	void ApplyFrameFromAnimation(uint32_t animationIndex, uint32_t frameIndex, eastl::vector<glm::mat4x4>& transforms);
	void ApplyInverseBindMatrices(uint32_t skeletonIndex, eastl::vector<glm::mat4x4>& transforms);

	uint32_t GetNumFramesForAnimation(uint32_t animationIndex);
private:
	const Definition::AnimationDatabase* m_Database;
};
}