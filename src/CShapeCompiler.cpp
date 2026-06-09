#include "CShapeCompiler.h"
#include "CBinaryFile.h"
#include "CScopeExit.h"
#include "U_Log.h"
#include "U_String.h"
#include "U_Files.h"

#include <fstream>
#include <vector>

#include "glm/glm.hpp"

bool CShapeCompiler::CompileShape(const std::filesystem::path& srcpath)
{
    std::ifstream objfile(srcpath);
    if (!objfile.is_open()) { Log::ErrInstance() << "Can't open source file\n"; return false; }

    CScopeExit streamExiter([&objfile]() { objfile.close(); });

    std::filesystem::path outpath = FileUtils::get_executable_path() / "resources" / "shapes" / (srcpath.stem().string() + ".eshp");
    CBinaryFile outfile(outpath, false); //closed on destructor

    if(!outfile.IsOpen()) { Log::ErrInstance() << "Can't open output file\n"; return false; }
    
    glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };
    int CurrentMaterial = -1;

    std::vector<glm::vec3> Vertices;

    struct Triangle
    {
        int MaterialID = -1;
        glm::uvec3 Indices;
    };

    std::vector<Triangle> Triangles;
    std::vector<std::string> MaterialsList;

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
        else if (ReadLine[0] == 'v' && (ReadLine[1] == ' ' || ReadLine[1] == '\t'))
        {
            float X, Y, Z;
            sscanf(ReadLine.c_str(), "v %f %f %f", &X, &Y, &Z);

            X *= Scale.x;
            Y *= Scale.y;
            Z *= Scale.z;

            Vertices.push_back({ X, Y, Z });
        }
        else if (ReadLine[0] == 'l')
        {
            ReadLine.erase(0, 2);
            bool found = std::find(MaterialsList.begin(), MaterialsList.end(), ReadLine) != MaterialsList.end();

            if (!found)
            {
                MaterialsList.push_back(ReadLine);
                Log::Instance() << "Added material " << ReadLine << "\n";
            }
            CurrentMaterial = std::distance(MaterialsList.begin(), std::find(MaterialsList.begin(), MaterialsList.end(), ReadLine));
        }
        else if (ReadLine[0] == 'm')
        {
            ReadLine.erase(0, 2);

            bool found = std::find(MaterialsList.begin(), MaterialsList.end(), ReadLine) != MaterialsList.end();
            if (!found)
            {
                MaterialsList.push_back(ReadLine);
                Log::Instance() << "Forced material " << ReadLine << "\n";
            }

            CurrentMaterial = std::distance(MaterialsList.begin(), std::find(MaterialsList.begin(), MaterialsList.end(), ReadLine));
        }
        else if (ReadLine[0] == 'f')
        {
            int vertex[3];
            int texture[3];
            int normal[3];
            sscanf(ReadLine.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &vertex[0], &texture[0], &normal[0], &vertex[1], &texture[1], &normal[1], &vertex[2], &texture[2], &normal[2]);
            Triangle triangle;

            for(int i = 0; i < 3; i++) { triangle.Indices[i] = vertex[i] - 1; } //converted to base-0
            triangle.MaterialID = CurrentMaterial;
            Triangles.push_back(triangle);
        }
    }

    outfile.Write<std::uint64_t>(0); //placeholder (file size)
    outfile.Write<std::uint64_t>(Vertices.size());
    outfile.Write<std::uint64_t>(MaterialsList.size());

    Log::Instance() << "Vertices size is " << Vertices.size() << Log::Endl;
    Log::Instance() << "MaterialsList size is " << MaterialsList.size() << Log::Endl;

    for(std::uint64_t i = 0; i < Vertices.size(); i++)
    {
        outfile.Write<glm::vec3>(Vertices[i]);
    }

    for(std::uint64_t i = 0; i < MaterialsList.size(); i++)
    {
        size_t seek = outfile.GetSeek();
        outfile.Write<std::uint64_t>(0); //placeholder (tris count)
        outfile.WriteLenString<std::uint8_t, std::string>(MaterialsList.at(i));

        std::uint64_t count = 0;
        for(std::uint64_t j = 0; j < Triangles.size(); j++)
        {
            auto& tri = Triangles.at(j);
            if(tri.MaterialID == i)
            {
                outfile.Write<glm::u64vec3>
                (
                    {
                        static_cast<glm::u64vec3::value_type>(tri.Indices.x),
                        static_cast<glm::u64vec3::value_type>(tri.Indices.y),
                        static_cast<glm::u64vec3::value_type>(tri.Indices.z)
                    }
                );
                count++;
            }
        }

        size_t curseek = outfile.GetSeek();
        outfile.SetSeek(seek);

        outfile.Write<std::uint64_t>(count); //written placeholder for tris count
        outfile.SetSeek(curseek);
    }

    size_t seek = outfile.GetSeek();
    outfile.SetSeek(0);
    
    size_t fileSize = seek - sizeof(std::uint64_t);
    Log::Instance() << "File size is " << fileSize << Log::Endl;

    outfile.Write<std::uint64_t>(fileSize);
    outfile.Close();

    Log::Instance() << "Collision shape compiled!\n";
    return true;
}
