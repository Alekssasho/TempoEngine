#include <CommonIncludes.h>

#include <World/World.h>
#include <World/Navigation.h>
#include <World/Components/Components.h>

#include <random>

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

LaneIterator FindLane(const Components::NavigationData& navData, glm::vec3 currentPos, glm::vec3 targetPos, uint64_t entitySeed)
{
    // TODO: Temp memory
    float totalScore = 0.0f;
    eastl::vector<float> scores;
    eastl::vector<LaneIterator> iterators;
    scores.reserve(navData.Lines.size());
    iterators.reserve(navData.Lines.size());
    for (uint32_t i = 0; i < navData.Lines.size(); ++i)
    {
        auto lanePoints = eastl::span(&navData.Points[navData.Lines[i].StartIndex], navData.Lines[i].Count);
        auto currentClosestPointIndex = FindClosestPointOnLane(lanePoints, currentPos);
        auto targetClosestPointIndex = FindClosestPointOnLane(lanePoints, targetPos);

        // Add little epsilon to avoid 0 scores
        float score = glm::distance2(lanePoints[currentClosestPointIndex], currentPos) + glm::distance2(lanePoints[targetClosestPointIndex], targetPos) + 0.001f;
        totalScore += score;
        scores.push_back(score);
        iterators.push_back(LaneIterator{
                .LaneIndex = i,
                .CurrentPointIndex = currentClosestPointIndex,
                .CurrentMode = currentClosestPointIndex < targetClosestPointIndex ? LaneIterator::Mode::Forward : LaneIterator::Mode::Backward,
            });
    }

    // Normalize and invert as we want smallest score
    for (uint32_t i = 0; i < scores.size(); ++i)
    {
        scores[i] /= totalScore;
        scores[i] = 1.0f - scores[i];
    }

    // Prefix sum
    float sum = 0.0f;
    for (uint32_t i = 0; i < scores.size(); ++i)
    {
        float currentScore = scores[i];
        scores[i] += sum;
        sum += currentScore;
    }

    uint64_t seed = entitySeed + GetGlobalSeed();

    using FastRandomEngine = std::linear_congruential_engine<uint64_t, 16807, 0, 2147483647>;
    std::ranlux48 rng(seed);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    float chosenValue = std::generate_canonical<float, 10>(rng);
    //FORMAT_LOG(Warning, Navigation, "Chooser %f, for %d id", chosenValue, entitySeed);
    auto chosenScore = eastl::lower_bound(scores.begin(), scores.end(), chosenValue);

    return iterators[eastl::distance(scores.begin(), chosenScore)];
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

void ReanchorToPosition(LaneIterator& itr, const Components::NavigationData& navData, glm::vec3 pos)
{
    if (itr.CurrentMode != LaneIterator::Mode::Forward && itr.CurrentMode != LaneIterator::Mode::Backward)
    {
        return;
    }

    auto lanePoints = eastl::span(&navData.Points[navData.Lines[itr.LaneIndex].StartIndex], navData.Lines[itr.LaneIndex].Count);
    if (lanePoints.size() < 2)
    {
        return;
    }

    const uint32_t closest = FindClosestPointOnLane(lanePoints, pos);
    const int32_t step = itr.CurrentMode == LaneIterator::Mode::Forward ? 1 : -1;
    const uint32_t lastPointIndex = itr.CurrentMode == LaneIterator::Mode::Forward ? uint32_t(lanePoints.size()) - 1 : 0;

    // If closest point already is the terminal waypoint, nothing to do.
    if (closest == lastPointIndex)
    {
        itr.CurrentPointIndex = closest;
        return;
    }

    // Decide whether the closest point lies ahead or behind us along the lane direction.
    // Segment direction from `closest` to the next point in travel order.
    const glm::vec3 segmentDir = lanePoints[closest + step] - lanePoints[closest];
    const glm::vec3 toPoint = lanePoints[closest] - pos;

    // Positive dot => point is "ahead" of pos along travel direction. Negative => behind; advance one step.
    if (glm::dot(toPoint, segmentDir) >= 0.0f)
    {
        itr.CurrentPointIndex = closest;
    }
    else
    {
        itr.CurrentPointIndex = uint32_t(int32_t(closest) + step);
    }
}
}
}