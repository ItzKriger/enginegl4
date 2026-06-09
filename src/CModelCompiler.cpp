#include "CModelCompiler.h"
#include "CBinaryFile.h"
#include "CModelLoadInfo.h"
#include "CScopeExit.h"
#include "U_Log.h"
#include "U_Files.h"

#include <fstream>

/*
class CModelLoadInfo
{
public:
    class CBone
    {
    public:
        int ParentIndex = -1;
        std::string Name;
        CTransform BindTransform;
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
*/

bool CModelCompiler::CompileModel(const std::filesystem::path& srcpath)
{
    CModelLoadInfo model;

    std::ifstream objfile(srcpath);
    if (!objfile.is_open()) { Log::ErrInstance() << "Can't open source file\n"; return false; }

    CScopeExit streamExiter([&objfile]() { objfile.close(); });

    std::filesystem::path outpath = FileUtils::get_executable_path() / "resources" / "models" / (srcpath.stem().string() + ".emdl");
    CBinaryFile outfile(outpath, false); //closed on destructor

    if(!outfile.IsOpen()) { Log::ErrInstance() << "Can't open output file\n"; return false; }
    
    glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };
    std::vector<CModelLoadInfo::CVertex::CBoneLink> CurrentBoneLinking;
    int CurrentMaterial = -1;

    std::string ReadLine;
    while (!objfile.eof())
    {
        std::getline(objfile, ReadLine, '\n');

        if (ReadLine.substr(0, 5) == "scale")
        {
            std::vector<std::string> splitted;
            StringUtils::split_str(ReadLine, ' ', splitted);

            float scl = atof(splitted[1].c_str());
            Scale = { scl, scl, scl };

            Log::Instance() << "Scaled to " << scl << "\n";
        }
        else if (ReadLine.substr(0, 6) == "scale3")
        {
            std::vector<std::string> splitted;
            StringUtils::split_str(ReadLine, ' ', splitted);

            float scl[3];
            for(int i = 0; i < 3; i++) { scl[i] = atof(splitted[i + 1].c_str()); }

            Scale = { scl[0], scl[1], scl[2] };
            Log::Instance() << "Scaled to { " << StringUtils::ToStr(Scale) << " }\n";
        }
        else if (ReadLine[0] == 'v')
        {
            if (ReadLine[1] == 't')
            {
                float X, Y;
                sscanf(ReadLine.c_str(), "vt %f %f", &X, &Y);
                model.UvCoords.push_back({ X, Y });
            }
            else if (ReadLine[1] == 'n')
            {
                float X, Y, Z;
                sscanf(ReadLine.c_str(), "vn %f %f %f", &X, &Y, &Z);
                model.Normals.push_back({ X, Y, Z });
            }
            else
            {
                float X, Y, Z;
                sscanf(ReadLine.c_str(), "v %f %f %f", &X, &Y, &Z);

                X *= Scale.x;
                Y *= Scale.y;
                Z *= Scale.z;

                CModelLoadInfo::CVertex vertex;

                vertex.BoneCount = CurrentBoneLinking.size();
                vertex.Bones = CurrentBoneLinking;
                vertex.Position = { X, Y, Z };

                model.Vertices.push_back(vertex);
            }
        }
        else if (ReadLine[0] == 'l')
        {
            ReadLine.erase(0, 2);
            bool found = std::find(model.MaterialsList.begin(), model.MaterialsList.end(), ReadLine) != model.MaterialsList.end();

            if (!found)
            {
                model.MaterialsList.push_back(ReadLine);
                Log::Instance() << "Added material " << ReadLine << "\n";
            }
            CurrentMaterial = std::distance(model.MaterialsList.begin(), std::find(model.MaterialsList.begin(), model.MaterialsList.end(), ReadLine));
        }
        else if (ReadLine[0] == 'm')
        {
            ReadLine.erase(0, 2);

            bool found = std::find(model.MaterialsList.begin(), model.MaterialsList.end(), ReadLine) != model.MaterialsList.end();
            if (!found)
            {
                model.MaterialsList.push_back(ReadLine);
                Log::Instance() << "Forced material " << ReadLine << "\n";
            }

            CurrentMaterial = std::distance(model.MaterialsList.begin(), std::find(model.MaterialsList.begin(), model.MaterialsList.end(), ReadLine));
        }
        else if (ReadLine[0] == 'f')
        {
            int vertex[3];
            int texture[3];
            int normal[3];
            sscanf(ReadLine.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &vertex[0], &texture[0], &normal[0], &vertex[1], &texture[1], &normal[1], &vertex[2], &texture[2], &normal[2]);
            CModelLoadInfo::CTriangle triangle;

            for(int i = 0; i < 3; i++) { triangle.VertexIndex[i] = vertex[i] - 1; }
            for(int i = 0; i < 3; i++) { triangle.UvIndex[i] = texture[i] - 1; }
            for(int i = 0; i < 3; i++) { triangle.NormalIndex[i] = normal[i] - 1; }

            triangle.MaterialIndex = CurrentMaterial;
            model.Triangles.push_back(triangle);
        }
        else if (ReadLine[0] == 'j')
        {
            if (ReadLine[1] == 'c')
            {
                ReadLine.erase(0, 3);

                std::vector<std::string> splitted;
                StringUtils::split_str(ReadLine, ' ', splitted);

                std::string boneName = splitted.front();
                splitted.erase(splitted.begin());

                std::string boneParentName;

                bool found = std::find_if(model.Bones.begin(), model.Bones.end(), [&boneName](auto& bone) -> bool
                {
                    return bone.Name == boneName;
                }) != model.Bones.end();

                if (found)
                {
                    continue;
                }

                CModelLoadInfo::CBone bone;
                bone.Name = boneName;

                size_t transformIndex = 1;

                if(!StringUtils::isNumber(splitted.front()))
                {
                    boneParentName = splitted.front();
                    splitted.erase(splitted.begin());

                    auto parentIt = std::find_if(model.Bones.begin(), model.Bones.end(), [&boneParentName](auto& bone) -> bool
                    {
                        return bone.Name == boneParentName;
                    });

                    if(parentIt == model.Bones.end())
                    {
                        Log::ErrInstance() << "Can't find parent bone \"" << boneParentName << "\" for \"" << bone.Name << "\"\n";
                        return false;
                    }

                    bone.ParentIndex = std::distance(model.Bones.begin(), parentIt);
                    transformIndex = 2;
                }

                //glm::vec3 Position, Scale;
                //glm::quat Rotation;

                glm::mat4 bindMatrix;

                for(size_t y = 0; y < glm::mat4::length(); y++)
                {
                    for(size_t x = 0; x < glm::mat4::length(); x++)
                    {
                        bindMatrix[x][y] = StringUtils::FromStr<float>(splitted.front());
                        splitted.erase(splitted.begin());
                    }
                }

                //for(size_t i = 0; i < Position.length(); i++) { Position[i] = StringUtils::FromStr<float>(splitted.front()); splitted.erase(splitted.begin()); }
                //for(size_t i = 0; i < Rotation.length(); i++) { Rotation[i] = StringUtils::FromStr<float>(splitted.front()); splitted.erase(splitted.begin()); }
                //for(size_t i = 0; i < Scale.length(); i++) { Scale[i] = StringUtils::FromStr<float>(splitted.front()); splitted.erase(splitted.begin()); }

                //bone.BindTransform.SetFromMatrix()
                //bone.BindTransform.SetPRS(Position, Rotation, Scale);

                bone.BindTransform = bindMatrix;
                model.Bones.push_back(bone);

                auto thisBoneIt = std::find_if(model.Bones.begin(), model.Bones.end(), [&boneName](auto& bone) -> bool
                {
                    return bone.Name == boneName;
                });

                CurrentBoneLinking.clear();
                
                CModelLoadInfo::CVertex::CBoneLink newLink;
                newLink.BoneIndex = std::distance(model.Bones.begin(), thisBoneIt);
                newLink.Weight = 1.0f;

                CurrentBoneLinking.push_back(newLink);
            }
            else
            {
                CurrentBoneLinking.clear();
                ReadLine.erase(0, 2);

                std::vector<std::string> splitted;
                StringUtils::split_str(ReadLine, ' ', splitted);

                size_t boneCount = 0;
                std::string boneName;
                float boneWeight = 1.0f;

                for(size_t i = 0; i < splitted.size(); i++)
                {
                    if(i % 2 == 0)
                    {
                        boneName = splitted[i];
                    }
                    else
                    {
                        boneWeight = StringUtils::FromStr<float>(splitted[i]);

                        auto boneIt = std::find_if(model.Bones.begin(), model.Bones.end(), [&boneName](auto& bone) -> bool
                        {
                            return bone.Name == boneName;
                        });

                        if(boneIt == model.Bones.end())
                        {
                            Log::ErrInstance() << "Can't find bone \"" << boneName << "\"\n";
                            return false;
                        }

                        CModelLoadInfo::CVertex::CBoneLink newLink;
                        newLink.BoneIndex = std::distance(model.Bones.begin(), boneIt);
                        newLink.Weight = boneWeight;

                        CurrentBoneLinking.push_back(newLink);
                    }
                }
            }
        }
    }

    size_t _dbg_MaterialIndex = 0, _dbg_BoneIndex = 0;

    outfile.Write<std::uint16_t>(model.MaterialsList.size());
    Log::Instance() << "Materials count: " << model.MaterialsList.size() << "\n";

    
    for(size_t i = 0; i < model.MaterialsList.size(); i++)
    {
        outfile.WriteString(model.MaterialsList.at(i));
        Log::Instance() << "Material #" << _dbg_MaterialIndex << " is " << model.MaterialsList.at(i) << "\n"; _dbg_MaterialIndex++;
    }

    outfile.Write<std::uint16_t>(model.Bones.size());
    Log::Instance() << "Bones count: " << model.Bones.size() << "\n";

    for(size_t i = 0; i < model.Bones.size(); i++)
    {
        outfile.Write(model.Bones.at(i).ParentIndex);
        outfile.WriteString(model.Bones.at(i).Name);

        outfile.Write(model.Bones.at(i).BindTransform);

        //outfile.Write(model.Bones.at(i).BindTransform.GetPosition());
        //outfile.Write(model.Bones.at(i).BindTransform.GetRotation());
        //outfile.Write(model.Bones.at(i).BindTransform.GetScale());

        Log::Instance() << "Bone #" << _dbg_BoneIndex << " is " << model.Bones.at(i).Name << " with parent index of " << model.Bones.at(i).ParentIndex << "\n"; _dbg_BoneIndex++;
    }

    outfile.Write<std::uint64_t>(model.UvCoords.size());
    Log::Instance() << "Uv coords count: " << model.UvCoords.size() << "\n";

    for(size_t i = 0; i < model.UvCoords.size(); i++)
    {
        outfile.Write(model.UvCoords.at(i));
    }

    outfile.Write<std::uint64_t>(model.Normals.size());
    Log::Instance() << "Normals count: " << model.Normals.size() << "\n";

    for(size_t i = 0; i < model.Normals.size(); i++)
    {
        outfile.Write(model.Normals.at(i));
    }

    outfile.Write<std::uint64_t>(model.Vertices.size());
    Log::Instance() << "Vertices count: " << model.Vertices.size() << "\n";

    for(size_t i = 0; i < model.Vertices.size(); i++)
    {
        outfile.Write(model.Vertices.at(i).Position);
        outfile.Write(model.Vertices.at(i).BoneCount);

        size_t _dbg_BoneCycles = 0;
        for(std::uint8_t j = 0; j < model.Vertices.at(i).BoneCount; j++)
        {
            outfile.Write(model.Vertices.at(i).Bones.at(j).BoneIndex);
            outfile.Write(model.Vertices.at(i).Bones.at(j).Weight);

            _dbg_BoneCycles++;
        }

        if(_dbg_BoneCycles != model.Vertices.at(i).BoneCount)
        {
            Log::Errln("Warning! There's mismatch in actual written bones count and BoneCount var!");
        }
    }

    outfile.Write<std::uint64_t>(model.MaterialsList.size());

    for(size_t i = 0; i < model.MaterialsList.size(); i++)
    {
        outfile.Write<int>(i);

        std::uint64_t count = 0;
        for(auto& tri : model.Triangles)
        {
            if(tri.MaterialIndex == i)
            {
                count++;
            }
        }

        Log::Instance() << "Writing " << count << " polygons for material #" << i << "\n";

        outfile.Write<std::uint64_t>(count);
        for(auto& tri : model.Triangles)
        {
            if(tri.MaterialIndex == i)
            {
                outfile.Write(tri.VertexIndex);
                outfile.Write(tri.UvIndex);
                outfile.Write(tri.NormalIndex);
            }
        }
    }

    Log::Instance() << "Model info loaded!\n";
    return true;
}
