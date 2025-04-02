#pragma once

#include <CommonIncludes.h>

namespace NavigationBuilder
{

struct Line
{
    void AddPoint(glm::vec3 p)
    {
        Points.push_back(p);
    }

    eastl::vector<glm::vec3> Points;
};

struct NavigationBuilder
{
public:
    Line& NewLine()
    {
        m_Lines.push_back();
        return m_Lines.back();
    }

    void Build(Tempest::Components::NavigationData& nav)
    {
        for (const auto& line : m_Lines)
        {
            auto startIndex = uint32_t(nav.Points.size());
            nav.Points.insert(nav.Points.end(), line.Points.begin(), line.Points.end());
            nav.Lines.emplace_back(startIndex, uint32_t(line.Points.size()));
        }
    }
private:
    eastl::vector<Line> m_Lines;
};
}