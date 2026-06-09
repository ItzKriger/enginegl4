#include "CModelLoadInfo.h"
#include "CBinaryFile.h"

#include "CScopeExit.h"
#include "U_String.h"
#include "U_Numbers.h"
#include "U_Log.h"
#include "U_Files.h"

CModelLoadInfo::CBone& CModelLoadInfo::CBone::GetParent(std::vector<CBone>& bones)
{
    auto it = std::find_if(bones.begin(), bones.end(), [this](const CBone& bone) -> bool { return bone.Name == ParentName; });
    return (*it);
}

//BONEMATRIX calculating bind pose here
void CModelLoadInfo::CBone::CalculateBind(std::vector<CBone>& bones)
{
    GlobalBind = glm::mat4x4();
    auto LocalBind = BindTransform;

    if(ParentIndex > -1)
    {
        GlobalBind = GetParent(bones).GlobalBind * LocalBind;
    }
    else
    {
        GlobalBind = LocalBind;
    }

    /*Log::Instance() << "Bone " << Name << " local bind is:\n";
    for(size_t y = 0; y < glm::mat4::length(); y++)
    {
        for(size_t x = 0; x < glm::mat4::length(); x++)
        {
            Log::Instance() << LocalBind[x][y] << " ";
        }
    }

    Log::Instance() << Log::Endl << Log::Endl;

    Log::Instance() << "Bone " << Name << " global bind is:\n";
    for(size_t y = 0; y < glm::mat4::length(); y++)
    {
        for(size_t x = 0; x < glm::mat4::length(); x++)
        {
            Log::Instance() << GlobalBind[x][y] << " ";
        }
    }

    Log::Instance() << Log::Endl << Log::Endl;*/
}

void CModelLoadInfo::Reset()
{
    Triangles.clear();
    Normals.clear();
    UvCoords.clear();
    Vertices.clear();
    Bones.clear();
    MaterialsList.clear();
}

bool CModelLoadInfo::CheckIndices(const glm::u64vec3& vec, size_t limit)
{
    return (vec.x < limit || vec.y < limit || vec.z < limit);
}

int CModelLoadInfo::CheckBoneValidity(CBone& bone)
{
    if(StringUtils::IsEmpty(bone.Name)) { return 1; }
    if(bone.ParentIndex < -1 || bone.ParentIndex >= static_cast<int>(Bones.size())) { return 2; }
    if (&bone - Bones.data() == bone.ParentIndex) { return 3; }

    int visitedCount = 0;
    int parentIndex = bone.ParentIndex;
    while (parentIndex != -1)
    {
        if (visitedCount++ > Bones.size())
        {
            return 4;
        }

        if (parentIndex == (&bone - Bones.data()))
        {
            return 5;
        }

        parentIndex = Bones[parentIndex].ParentIndex;
    }

    if (bone.ParentIndex >= 0 && bone.ParentIndex >= static_cast<int>(&bone - Bones.data()))
    {
        return 6;
    }

    return 0;
}

bool CModelLoadInfo::Load(const std::filesystem::path& path)
{
    CScopeExit guard([this]() { Reset(); });
    auto Error = [](const std::string& err) -> bool { Log::Errln(StringUtils::StrToWstr(err)); return false; };

    auto found_path = FileUtils::find_first_by_name(FileUtils::get_executable_path() / "resources" / "models", path.string());
    if(!found_path.has_value()) { return Error("Couldn't find file " + path.string()); }

    CBinaryFile bin;
    bin.OpenRead(found_path.value()); //file closes itself on ~CBinaryFile
    
    if(!bin.IsOpen()) { return Error("Couldn't open file " + found_path.value().string()); }
    
    std::uint16_t MaterialsCount, BonesCount;
    std::uint64_t VerticesCount, UvCoordsCount, NormalsCount, MaterialLinksCount;
    
    bin.Read(MaterialsCount);
    MaterialsList.reserve(MaterialsCount);

    //Log::Instance() << "Materials count: " << MaterialsCount << "\n"; LOGLOG

    for(std::uint16_t i = 0; i < MaterialsCount; i++)
    {
        std::string matname = bin.ReadString();
        //Log::Instance() << "Got material name: \"" << matname << "\"\n"; LOGLOG

        if(!StringUtils::IsEmpty(matname))
        {
            MaterialsList.push_back(matname);
        }
    }

    bin.Read(BonesCount);
    Bones.reserve(BonesCount);

    //Log::Instance() << "Bones count: " << BonesCount << "\n"; LOGLOG

    for(std::uint16_t i = 0; i < BonesCount; i++)
    {
        CBone Bone;

        bin.Read(Bone.ParentIndex);
        bin.ReadString(Bone.Name);

        //Log::Instance() << "Got bone named \"" << Bone.Name << "\" with parent index of " << Bone.ParentIndex << "\n"; LOGLOG
        
        //CTransform::CMeasurePack transform_pack;

        //bin.Read(transform_pack.Position);
        //bin.Read(transform_pack.Rotation);
        //bin.Read(transform_pack.Scale);

        bin.Read(Bone.BindTransform);

        //Bone.BindTransform.SetPRS(transform_pack);
        Bones.push_back(Bone);
    }

    for(auto& bone : Bones)
    {
        int status = CheckBoneValidity(bone);
        if(status != 0)
        {
            return Error("Invalid bone with the status of " + std::to_string(status) + " was detected at \"" + bone.Name + "\"");
        }
        else
        {
            if(bone.ParentIndex > -1)
            {
                bone.ParentName = Bones.at(bone.ParentIndex).Name;
            }
            bone.CalculateBind(Bones);
        }
    }

    bin.Read(UvCoordsCount);
    UvCoords.reserve(UvCoordsCount);

    //Log::Instance() << "Uv coords count: " << UvCoordsCount << "\n"; LOGLOG

    for(std::uint64_t i = 0; i < UvCoordsCount; i++)
    {
        UvCoords.push_back(bin.Read<glm::vec2>());
    }

    bin.Read(NormalsCount);
    Normals.reserve(NormalsCount);

    //Log::Instance() << "Normals count: " << NormalsCount << "\n"; LOGLOG

    for(std::uint64_t i = 0; i < NormalsCount; i++)
    {
        Normals.push_back(bin.Read<glm::vec3>());
    }

    bin.Read(VerticesCount);
    Vertices.reserve(VerticesCount);

    //Log::Instance() << "Vertices count: " << VerticesCount << "\n"; LOGLOG

    for(std::uint64_t i = 0; i < VerticesCount; i++)
    {
        CVertex vertex;

        bin.Read(vertex.Position);
        bin.Read(vertex.BoneCount);

        float TotalWeight = vertex.BoneCount == 0 ? 1.0f : 0.0f;
        for(std::uint8_t j = 0; j < vertex.BoneCount; j++)
        {
            CVertex::CBoneLink link;

            bin.Read(link.BoneIndex);
            bin.Read(link.Weight);
            
            if(link.BoneIndex < 0) { return Error("Vertex " + std::to_string(i) + " has invalid bone link"); }
            TotalWeight += link.Weight;

            vertex.Bones.push_back(link);
        }

        if(!almost_equal(TotalWeight, 1.0f, 0.1f))
        {
            return Error("Bad total weight of " + std::to_string(TotalWeight) + " at vertex " + std::to_string(i));
        }
        Vertices.push_back(vertex);
    }

    bin.Read(MaterialLinksCount);
    //Log::Instance() << "Material links count: " << MaterialLinksCount << "\n"; LOGLOG

    for(std::uint64_t i = 0; i < MaterialLinksCount; i++)
    {
        int materialIndex = -1;
        bin.Read(materialIndex);

        if(materialIndex < -1 || materialIndex >= static_cast<int>(MaterialsList.size()))
        {
            return Error("Invalid material index (" + std::to_string(materialIndex) + ") with available range of [-1; " + std::to_string(static_cast<int>(MaterialsList.size())) + "]");
        }

        std::uint64_t TrianglesCount;
        bin.Read(TrianglesCount);

        for(std::uint64_t i = 0; i < TrianglesCount; i++)
        {
            CTriangle triangle;

            bin.Read(triangle.VertexIndex);
            bin.Read(triangle.UvIndex);
            bin.Read(triangle.NormalIndex);

            if(!CheckIndices(triangle.VertexIndex, Vertices.size()))
            {
                return Error("Invalid vertex index (" +
                    std::to_string(triangle.VertexIndex.x) + ", " + std::to_string(triangle.VertexIndex.y) + ", " + std::to_string(triangle.VertexIndex.z) +
                     ") with available range of [0; " + std::to_string(static_cast<int>(Vertices.size())) + "]");
                //return Error("Invalid vertex index");
            }

            if(!CheckIndices(triangle.UvIndex, UvCoords.size()))
            {
                return Error("Invalid UV coord index (" +
                    std::to_string(triangle.UvIndex.x) + ", " + std::to_string(triangle.UvIndex.y) + ", " + std::to_string(triangle.UvIndex.z) +
                     ") with available range of [0; " + std::to_string(static_cast<int>(UvCoords.size())) + "]");
                //return Error("Invalid UV coord index");
            }

            if(!CheckIndices(triangle.NormalIndex, Normals.size()))
            {
                return Error("Invalid normal vector index (" +
                    std::to_string(triangle.NormalIndex.x) + ", " + std::to_string(triangle.NormalIndex.y) + ", " + std::to_string(triangle.NormalIndex.z) +
                     ") with available range of [0; " + std::to_string(static_cast<int>(Normals.size())) + "]");
                //return Error("Invalid normal vector index");
            }
            
            triangle.MaterialIndex = materialIndex;
            Triangles.push_back(triangle);
        }
    }

    guard.Dismiss();
    return true;
}
