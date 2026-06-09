#include "CAnimationCompiler.h"
#include "CBinaryFile.h"
#include "CScopeExit.h"
#include "U_Log.h"
#include "U_Files.h"

#include <fstream>

bool CAnimationCompiler::CompileAnimation(const std::filesystem::path& srcpath)
{
    std::ifstream objfile(srcpath);
    if (!objfile.is_open()) { return false; }

    CScopeExit streamExiter([&objfile]() { objfile.close(); });

    std::filesystem::path outpath = FileUtils::get_executable_path() / "resources" / "anim" / (srcpath.stem().string() + ".eanm");
    CBinaryFile outfile(outpath, false); //closed on destructor

    if(!outfile.IsOpen()) { return false; }
    
    size_t FramesCount = 100, BonesCount = 1;
    double FPS = 30.0;

    struct Bone
    {
        std::string Name;
        glm::mat4 Matrix = glm::mat4(1.0f);
    };

    struct Frame
    {
        size_t FrameIndex = 0;
        std::vector<std::string> Events; //events are being fired with interp argument and time of anim

        std::vector<Bone> Bones;

        void reset()
        {
            FrameIndex = 0;
            Events.clear();
            Bones.clear();
        }
    };

    std::vector<Frame> Frames;
    Frame curFrame;

    std::string ReadLine;
    while (!objfile.eof())
    {
        std::getline(objfile, ReadLine, '\n');
        if(StringUtils::IsEmpty(ReadLine) || ReadLine.at(0) == '#') { continue; }

        if(StringUtils::StartsWith(ReadLine, "frames:"))
        {
            auto splitted = StringUtils::split_str(ReadLine, ':');
            if(splitted.size() < 2) { continue; }

            auto removed = StringUtils::RemoveSpaces(splitted.at(1));
            FramesCount = StringUtils::FromStr<size_t>(removed);
        }
        else if(StringUtils::StartsWith(ReadLine, "fps:"))
        {
            auto splitted = StringUtils::split_str(ReadLine, ':');
            if(splitted.size() < 2) { continue; }

            auto removed = StringUtils::RemoveSpaces(splitted.at(1));
            FPS = StringUtils::FromStr<double>(removed);
        }
        else if(StringUtils::StartsWith(ReadLine, "bones:"))
        {
            auto splitted = StringUtils::split_str(ReadLine, ':');
            if(splitted.size() < 2) { continue; }

            auto removed = StringUtils::RemoveSpaces(splitted.at(1));
            BonesCount = StringUtils::FromStr<size_t>(removed);
        }
        else if(StringUtils::StartsWith(ReadLine, "events:"))
        {
            if(!curFrame.Bones.empty()) //valid frame
            {
                curFrame.FrameIndex = Frames.size();

                Frames.push_back(std::move(curFrame)); //std::move?
                Log::Instance() << "Added frame #" << Frames.size() - 1 << Log::Endl;

                curFrame.reset();
            }
            else
            {
                Log::Instance() << "empty bones" << Log::Endl;
            }

            auto splitted = StringUtils::split_str(ReadLine, ':');

            if(splitted.size() < 2) { continue; }
            if(StringUtils::IsEmpty(splitted.at(1))) { continue; }

            auto removed = StringUtils::RemoveSpaces(splitted.at(1));

            auto splitted_spaces = StringUtils::split_str(removed, ' ');
            for(auto& s : splitted_spaces)
            {
                curFrame.Events.push_back(s);
                Log::Instance() << "Added event \"" << s << "\"\n";
            }
        }
        else
        {
            auto removed = StringUtils::RemoveSpaces(ReadLine);
            auto splitted = StringUtils::split_str(removed, ' ');

            if(splitted.size() != 17) { continue; }

            Bone bone;
            bone.Name = splitted.at(0);

            for (size_t y = 0; y < glm::mat4::length(); y++)
            {
                for (size_t x = 0; x < glm::mat4::length(); x++)
                {
                    bone.Matrix[x][y] = StringUtils::FromStr<float>(splitted.at((x + (y * glm::mat4::length())) + 1));
                }
            }

            //Log::Instance() << "at Frame #" << Frames.size() << Log::Endl;
            //Log::Instance() << "Bone \"" << bone.Name << "\" with matrix " << glm::determinant(bone.Matrix) << Log::Endl;

            curFrame.Bones.push_back(std::move(bone));
        }
    }

    if(!curFrame.Bones.empty()) //valid frame
    {
        curFrame.FrameIndex = Frames.size();

        Frames.push_back(std::move(curFrame)); //std::move?
        Log::Instance() << "Added frame #" << Frames.size() - 1 << Log::Endl;
    }

    //FramesCount isn't necessary to be stated manually
    outfile.Write<std::uint32_t>(Frames.size());
    outfile.Write<double>(FPS);
    //outfile.Write<std::uint16_t>(BonesCount); //bones count doesn't matter

    for(auto& frame : Frames)
    {
        outfile.Write<std::uint8_t>(frame.Events.size());
        outfile.Write<std::uint16_t>(frame.Bones.size());

        for(auto& event : frame.Events)
        {
            outfile.WriteLenString<std::uint8_t>(event);
        }

        for(auto& bone : frame.Bones)
        {
            outfile.WriteLenString<std::uint8_t>(bone.Name);
            outfile.Write<glm::mat4>(bone.Matrix);
        }
    }

    Log::Instance() << "FramesCount var is " << FramesCount << Log::Endl;
    Log::Instance() << "Actual frames count is " << Frames.size() << Log::Endl;
    Log::Instance() << "FPS is " << FPS << Log::Endl;
    Log::Instance() << "Bones count is " << BonesCount << Log::Endl;
    
    Log::Instance() << "Animation compiled!\n";
    return true;
}
