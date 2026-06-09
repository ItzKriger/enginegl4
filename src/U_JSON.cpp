#include "U_JSON.h"

void JsonToVHACD(const nlohmann::json& _json, VHACD::IVHACD::Parameters& params)
{
    params.m_maxConvexHulls = GetFromJson<uint32_t>(_json, "maxConvexHulls", params.m_maxConvexHulls);
    params.m_resolution = GetFromJson<uint32_t>(_json, "resolution", params.m_resolution);
    params.m_minimumVolumePercentErrorAllowed = GetFromJson<double>(_json, "volumePercentError", params.m_minimumVolumePercentErrorAllowed);
    params.m_maxRecursionDepth = GetFromJson<uint32_t>(_json, "maxRecursionDepth", params.m_maxRecursionDepth);
    params.m_shrinkWrap = GetFromJson<bool>(_json, "shrinkWrap", params.m_shrinkWrap);
    params.m_maxNumVerticesPerCH = GetFromJson<uint32_t>(_json, "maxVerticesPerChunk", params.m_maxNumVerticesPerCH);
    params.m_minEdgeLength = GetFromJson<uint32_t>(_json, "minEdgeLength", params.m_minEdgeLength);
    params.m_findBestPlane = GetFromJson<bool>(_json, "findBestPlane", params.m_findBestPlane);

    if(_json.contains("fillMode"))
    {
        auto& fillMode = _json["fillMode"];
        if(fillMode.is_string())
        {
            auto sFillMode = fillMode.get<std::string>();
            if(sFillMode == "FLOOD_FILL")
            {
                params.m_fillMode = VHACD::FillMode::FLOOD_FILL;
            }
            else if(sFillMode == "SURFACE_ONLY")
            {
                params.m_fillMode = VHACD::FillMode::SURFACE_ONLY;
            }
            else if(sFillMode == "RAYCAST_FILL")
            {
                params.m_fillMode = VHACD::FillMode::RAYCAST_FILL;
            }
        }
    }
}
