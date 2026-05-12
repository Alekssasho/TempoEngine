#pragma once

namespace Tempest
{
namespace Components { struct NavigationData; }

namespace Navigation
{
struct LineData
{
    uint32_t StartIndex;
    uint32_t Count;
};

struct LaneIterator
{
    enum class Mode : uint32_t
    {
        Invalid,
        Forward,
        Backward,
        Finished,
    };

    uint32_t LaneIndex = 0;
    uint32_t CurrentPointIndex = 0;
    Mode CurrentMode = Mode::Invalid;

    bool IsValid();
    glm::vec3 UpdateNextDirection(const Components::NavigationData& navData, glm::vec3 pos);
};

// NB: enum with mode of finding could be added, if different ways are needed
// for now we go with creating CDF of distance score and getting random
LaneIterator FindLane(const Components::NavigationData& navData, glm::vec3 currentPos, glm::vec3 targetPos, uint64_t seed);

// Snaps CurrentPointIndex to a lane point that still lies ahead of pos in the iterator's
// current direction. Leaves LaneIndex and CurrentMode untouched. No-op unless mode is Forward/Backward.
void ReanchorToPosition(LaneIterator& itr, const Components::NavigationData& navData, glm::vec3 pos);
}
}