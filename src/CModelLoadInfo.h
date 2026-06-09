#pragma once
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

#include "CTransform.h"

class CModelLoadInfo
{
public:
    class CBone
    {
    public:
        int ParentIndex = -1;
        std::string Name, ParentName;

        glm::mat4x4 BindTransform;
        glm::mat4x4 GlobalBind;

        void CalculateBind(std::vector<CBone>& bones);
        CBone& GetParent(std::vector<CBone>& bones);
    };

    class CVertex
    {
    public:
        class CBoneLink
        {
        public:
            float Weight = 1.0f;
            int BoneIndex = -1;
        };

        //TODO vertex color

        glm::vec3 Position;
        std::uint8_t BoneCount = 0;

        std::vector<CBoneLink> Bones;
    };
    
    class CTriangle
    {
    public:
        glm::u64vec3 VertexIndex;
        glm::u64vec3 UvIndex;
        glm::u64vec3 NormalIndex;

        int MaterialIndex = -1;
    };

    std::vector<CBone> Bones;
    std::vector<CVertex> Vertices;
    std::vector<glm::vec2> UvCoords;
    std::vector<glm::vec3> Normals;
    std::vector<CTriangle> Triangles;
    std::vector<std::string> MaterialsList;

    void Reset();
    bool Load(const std::filesystem::path& path);
    int CheckBoneValidity(CBone& bone);
    bool CheckIndices(const glm::u64vec3& vec, size_t limit);
};
