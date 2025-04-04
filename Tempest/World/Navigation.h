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

LaneIterator FindClosestLane(const Components::NavigationData& navData, glm::vec3 currentPos, glm::vec3 targetPos);
}
}