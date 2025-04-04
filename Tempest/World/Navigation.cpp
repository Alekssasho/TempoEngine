#include <CommonIncludes.h>

#include <World/Navigation.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace Navigation
{

uint32_t FindClosestPointOnLane(eastl::span<const glm::vec3> points, glm::vec3 pos)
{
    uint32_t index = 0;
    float minDistance = std::numeric_limits<float>::max();
    for (uint32_t i = 0; i < points.size(); ++i)
    {
        float distance = glm::distance2(points[i], pos);
        if (distance < minDistance)
        {
            minDistance = distance;
            index = i;
        }
    }
    return index;
}

LaneIterator FindClosestLane(const Components::NavigationData& navData, glm::vec3 currentPos, glm::vec3 targetPos)
{
    float minScore = std::numeric_limits<float>::max();
    LaneIterator result;
    for (uint32_t i = 0; i < navData.Lines.size(); ++i)
    {
        auto lanePoints = eastl::span(&navData.Points[navData.Lines[i].StartIndex], navData.Lines[i].Count);
        auto currentClosestPointIndex = FindClosestPointOnLane(lanePoints, currentPos);
        auto targetClosestPointIndex = FindClosestPointOnLane(lanePoints, targetPos);

        float score = glm::distance2(lanePoints[currentClosestPointIndex], currentPos) + glm::distance2(lanePoints[targetClosestPointIndex], targetPos);
        if (score < minScore)
        {
            minScore = score;
            result = LaneIterator{
                .LaneIndex = i,
                .CurrentPointIndex = currentClosestPointIndex,
                .CurrentMode = currentClosestPointIndex < targetClosestPointIndex ? LaneIterator::Mode::Forward : LaneIterator::Mode::Backward,
            };
        }
    }

    return result;
}

bool LaneIterator::IsValid()
{
    return CurrentMode != Mode::Invalid && CurrentMode != Mode::Finished;
}

glm::vec3 LaneIterator::UpdateNextDirection(const Components::NavigationData& navData, glm::vec3 pos)
{
    assert(CurrentMode == Mode::Forward || CurrentMode == Mode::Backward);
    auto lanePoints = eastl::span(&navData.Points[navData.Lines[LaneIndex].StartIndex], navData.Lines[LaneIndex].Count);

    const uint32_t lastPointIndex = CurrentMode == Mode::Forward ? uint32_t(lanePoints.size()) - 1 : 0;

    constexpr float epsilonDistance = 0.2f * 0.2f;
    if (glm::distance2(pos, lanePoints[CurrentPointIndex]) < epsilonDistance)
    {
        if (CurrentPointIndex == lastPointIndex)
        {
            CurrentMode = Mode::Finished;
            return glm::vec3(0.0f, 0.0f, 0.0f);
        }

        CurrentPointIndex += CurrentMode == Mode::Forward ? 1 : -1;
    }

    return glm::normalize(lanePoints[CurrentPointIndex] - pos);
}
}
}