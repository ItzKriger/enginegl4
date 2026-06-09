#pragma once
#include <vector>
#include <map>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

enum class InterpolationType
{
    Linear,
    Cosine,
    Cubic
};

template <typename T>
struct SplineMath
{
    static T InterpolateLinear(const T& a, const T& b, float t)
    {
        return glm::mix(a, b, t);
    }

    static T InterpolateCubic(const T& p0, const T& p1, const T& p2, const T& p3, float t)
    {
        return glm::catmullRom(p0, p1, p2, p3, t);
    }
};

template <>
struct SplineMath<glm::quat>
{
    static glm::quat InterpolateLinear(const glm::quat& a, const glm::quat& b, float t)
    {
        return glm::slerp(a, b, t);
    }

    static glm::quat InterpolateCubic(const glm::quat& p0, const glm::quat& p1, const glm::quat& p2, const glm::quat& p3, float t)
    {
        glm::quat res = glm::catmullRom(p0, p1, p2, p3, t);
        return glm::normalize(res); 
    }
};

template<typename TKey, typename TValue>
class CSpline
{
public:
    void AddNode(TKey x, TValue y)
    {
        Nodes[x] = y;
    }

    TValue GetValue(TKey x, InterpolationType type = InterpolationType::Linear) const
    {
        if (Nodes.empty()) { return TValue(); }
        if (Nodes.size() == 1) { return Nodes.begin()->second; }

        auto it = Nodes.lower_bound(x);
        if (it == Nodes.begin()) { return it->second; }
        if (it == Nodes.end()) { return std::prev(Nodes.end())->second; }

        auto p2_it = it;
        auto p1_it = std::prev(it);

        TKey t_raw = (x - p1_it->first) / (p2_it->first - p1_it->first);
        float t = static_cast<float>(t_raw);

        switch (type)
        {
            case InterpolationType::Linear:
                return SplineMath<TValue>::InterpolateLinear(p1_it->second, p2_it->second, t);

            case InterpolationType::Cosine:
            {
                float t_smooth = glm::smoothstep(0.0f, 1.0f, t);
                return SplineMath<TValue>::InterpolateLinear(p1_it->second, p2_it->second, t_smooth);
            }

            case InterpolationType::Cubic:
            {
                const TValue& p1 = p1_it->second;
                const TValue& p2 = p2_it->second;
                
                const TValue& p0 = (p1_it == Nodes.begin()) ? p1 : std::prev(p1_it)->second;
                auto p3_it = std::next(p2_it);
                const TValue& p3 = (p3_it == Nodes.end()) ? p2 : p3_it->second;

                return SplineMath<TValue>::InterpolateCubic(p0, p1, p2, p3, t);
            }
        }
        return TValue();
    }

private:
    std::map<TKey, TValue> Nodes;
};
