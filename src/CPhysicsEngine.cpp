#include "CPhysicsEngine.h"
#include "CEngine.h"
#include "CGame.h"
#include "CResourcesManager.h"
#include "CConVarManager.h"
#include "CBinaryFile.h"
#include "U_Files.h"
#include "CBufferWrapper.h"
#include "CScopeExit.h"
#include "U_JSON.h"

#include "U_General.h"
#include "VHACD.h"

CPhysicsWorld* CPhysicsEngine::CreateWorld()
{
    auto vworld = V_CreateWorld();
    if(!vworld) { return nullptr; }

    vworld->OverridePersistent(this); //TODO should we do it here?

    glm::vec3 Gravity = DefaultGravity;
    COMPONENT_CALL_GET(Gravity, CConVarManager, GetConVarValue<glm::vec3>("phys.gravity", DefaultGravity));

    m_Worlds.push_back(std::move(vworld));
    auto& world = m_Worlds.back();

    world->SetGravity(Gravity);

    //mgr->AddConVar("phys.gravity", new CWrapable<glm::vec3>(DefaultGravity));
    //mgr->AddConVar("phys.substeps", new CWrapable<int>(10));
	//mgr->AddConVar("phys.fixedtimestep", new CWrapable<int>(60));

    return world.get();
}

void PhysicsShapes::CBasic::Load(const nlohmann::json& _json) { V_Load(_json); Build(); }
void PhysicsShapes::CBasic::V_Load(const nlohmann::json& _json) {}

void PhysicsShapes::CBox::V_Load(const nlohmann::json& _json)
{
    HalfExtents = GetFromJson(_json, "halfextents", glm::vec3(1.0f, 1.0f, 1.0f));
}

void PhysicsShapes::CCapsule::V_Load(const nlohmann::json& _json)
{
    Radius = GetFromJson(_json, "radius", 1.0f);
    Height = GetFromJson(_json, "height", 2.0f);
}

void PhysicsShapes::CSphere::V_Load(const nlohmann::json& _json)
{
    Radius = GetFromJson(_json, "radius", 1.0f);
}

void PhysicsShapes::CTriMesh::LoadTrimesh(const std::filesystem::path& path)
{
    Log::Instance() << "Loading trimesh " << path.string() << Log::Endl;

    auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
    auto _trimesh = resman->GetOrCreate("trimesh", path.string());

    _trimesh->Wait();
    auto trimesh = dynamic_cast<CTriangleMesh*>(_trimesh.get());

    for(auto& v : trimesh->Vertices)
    {
        Vertices.push_back(v);

        if(v.x >= 10000.0f || v.y >= 10000.0f || v.z >= 10000.0f)
        {
            Log::ErrInstance() << "AT LOADTRIMESH TOO FAR AWAY " << v << Log::Endl;
        }
    }

    for(auto& tri : trimesh->Triangles)
    {
        Triangles.push_back
        (
            {
                static_cast<glm::uvec3::value_type>(tri.Index.x),
                static_cast<glm::uvec3::value_type>(tri.Index.y),
                static_cast<glm::uvec3::value_type>(tri.Index.z)
            }
        );
    }
    Log::Instance() << "Loaded" << Log::Endl;
}

void PhysicsShapes::CTriMesh::V_Load(const nlohmann::json& _json)
{
    std::string path;
    path = GetFromJson<std::string>(_json, "mesh_path", "");

    LoadTrimesh(path);
}

void PhysicsShapes::CConvex::V_Load(const nlohmann::json& _json)
{
    std::string path;
    path = GetFromJson<std::string>(_json, "mesh_path", "");

    Log::Instance() << "Loading " << path << " for convex\n";
    LoadTrimesh(path);
}

void PhysicsShapes::CCompound::V_Load(const nlohmann::json& _json)
{
    auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
    if(!game) { return; }

    if(_json.contains("shapes"))
    {
        auto& val = _json["shapes"];
        if(val.is_array())
        {
            for(auto& kv : val.items())
            {
                if(kv.value().is_object())
                {
                    auto& obj = kv.value();

                    CTransform trans;
                    auto _shape = CCollision::ParseShape(obj, &trans);
                    auto shape = std::shared_ptr<PhysicsShapes::CBasic>(_shape.release()); //TODO why shared

                    Childs.push_back({ trans, shape });
                }
            }
        }
    }
}

void CWorldUnit::SetCollision(std::shared_ptr<CCollision> _col)
{
    V_SetCollision(_col);
    m_Collision = _col;
}

std::shared_ptr<CCollision> CWorldUnit::GetCollision() const { return m_Collision; }
void CWorldUnit::V_SetCollision(std::shared_ptr<CCollision> _col) {}

void CRigidBody::SetMass(float mass)
{
    V_SetMass(mass);
    m_Mass = mass;
}

float CRigidBody::GetMass() const { return m_Mass; }
void CRigidBody::V_SetMass(float mass) {}

void CWorldUnit::SetTransform(const CTransform& trans)
{
    Transform = trans;
}

bool CWorldUnit::IsUpdatingInternalTransform() const
{
    return m_updatingInternalTransform;
}

void CWorldUnit::UpdateInternalTransform()
{
    auto newTrans = GetInternalTransform();

    auto newMatrix = newTrans.GetModelMatrix();
    auto matrix = Transform.GetModelMatrix();

    if(newMatrix != matrix) //TODO may be inefficient
    {
        m_updatingInternalTransform = true;
        Transform = GetInternalTransform();
        m_updatingInternalTransform = false;
    }
}

void CWorldUnit::m_setCollisionPtr(std::shared_ptr<CCollision> _col)
{
    m_Collision = _col;
}

CTransform CWorldUnit::GetInternalTransform()
{
    return GetTransform();
}

CTransform CWorldUnit::GetTransform() const
{
    return Transform;
}

void CRigidBody::SetLinearVelocity(const glm::vec3& vel) {}
void CRigidBody::SetAngularVelocity(const glm::vec3& ang_vel) {}

glm::vec3 CRigidBody::GetLinearVelocity() const { return { 0.0f, 0.0f, 0.0f }; }
glm::vec3 CRigidBody::GetAngularVelocity() const { return { 0.0f, 0.0f, 0.0f }; }

void CRigidBody::Activate() {}

CCallbackHandler<void, CTransformBase::CMeasurePack, CTransformBase*>& CWorldUnit::GetTransformCallback()
{
    return Transform.OnTransformChanged;
}

std::unique_ptr<CPhysicsWorld> CPhysicsEngine::V_CreateWorld() { return nullptr; }

std::unique_ptr<CRigidBody> CPhysicsWorld::V_CreateRigidBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform, float mass)
{
    return nullptr;
}

std::unique_ptr<CKinematicBody> CPhysicsWorld::V_CreateKinematicBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform)
{
    return nullptr;
}

std::unique_ptr<CGhostBody> CPhysicsWorld::V_CreateGhostBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform)
{
    return nullptr;
}

float PhysicsShapes::CBasic::GetMargin() const { return 0.0f; }
void PhysicsShapes::CBasic::SetMargin(float _margin) {}

void CPhysicsWorld::V_AddRigidBody(CRigidBody* rgBody) {}
void CPhysicsWorld::V_AddKinematicBody(CKinematicBody* knBody) {}
void CPhysicsWorld::V_AddGhostBody(CGhostBody* ghBody) {}

CRigidBody* CPhysicsWorld::CreateRigidBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform, float mass)
{
    auto rgBody = V_CreateRigidBody(_collision, startTransform, mass);
    auto ret = rgBody.get();

    V_AddRigidBody(rgBody.get());
    rgBody->OverridePersistent(this);

    Units.push_back(std::move(rgBody));
    return ret;
}

CKinematicBody* CPhysicsWorld::CreateKinematicBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform)
{
    auto knBody = V_CreateKinematicBody(_collision, startTransform);
    auto ret = knBody.get();

    V_AddKinematicBody(knBody.get());
    knBody->OverridePersistent(this);

    Units.push_back(std::move(knBody));
    return ret;
}

CGhostBody* CPhysicsWorld::CreateGhostBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform)
{
    auto ghBody = V_CreateGhostBody(_collision, startTransform);
    auto ret = ghBody.get();

    V_AddGhostBody(ghBody.get());
    ghBody->OverridePersistent(this);

    Units.push_back(std::move(ghBody));
    return ret;
}

void CPhysicsEngine::Trace(std::function<void(CPhysicsWorld*)> func)
{
    for(auto& world : m_Worlds)
    {
        func(world.get());
    }
}

void CPhysicsWorld::SetGravity(const glm::vec3& _gravity)
{
    V_SetGravity(_gravity);
}

glm::vec3 CPhysicsWorld::GetGravity() const { return { 0.0f, 0.0f, 0.0f }; }

void CPhysicsWorld::V_SetGravity(const glm::vec3& _gravity)
{

}

void CPhysicsWorld::OverridePersistent(CPhysicsEngine* _engine) { m_linkedEngine = _engine; }
void CPhysicsWorld::V_Step(float deltaTime) {}

void CPhysicsWorld::Step(float deltaTime)
{
    V_Step(deltaTime);
    for(auto& unit : Units)
    {
        unit->UpdateInternalTransform(); //TODO may be ineffecient

        //update internal ->
        //transformable onchange
    }
}

void PhysicsShapes::CBasic::V_BaseBuild() {}
void PhysicsShapes::CBasic::V_Build() {}

void PhysicsShapes::CTriMesh::Merge(float threshold)
{
    /*std::vector<glm::vec3> merged;
    std::vector<bool> processed(Vertices.size(), false);

    for(size_t i1 = 0; i1 < Vertices.size(); i1++)
    {
        if(processed[i1]) { continue; }
        
        auto& v1 = Vertices[i1];
        auto accum = v1;
        size_t times = 1;
        processed[i1] = true;

        for(size_t i2 = i1 + 1; i2 < Vertices.size(); i2++)
        {
            if(processed[i2]) continue;
            
            auto& v2 = Vertices[i2];
            auto dist = glm::distance(v1, v2);
            if(dist <= threshold)
            {
                accum += v2;
                times++;
                processed[i2] = true;
            }
        }

        auto avg = accum / static_cast<float>(times);
        merged.push_back(avg);
    }*/

    Build();
}

void PhysicsShapes::CBasic::Build()
{
    V_BaseBuild();
    V_Build();
}

void PhysicsShapes::CConvex::V_BaseBuild()
{
    if(NeedVHACD)
    {
        BuildConvex();
    }
}

void PhysicsShapes::CCompound::V_BaseBuild()
{
    for(auto& ch : Childs)
    {
        //ch.Shape->Build(); //TODO WHY BUILD
    }
}

CPhysicsEngine::CPhysicsEngine()
{
    ShapesFactory.add<PhysicsShapes::CTriMesh>("trimesh");
    ShapesFactory.add<PhysicsShapes::CConvex>("convex");
    ShapesFactory.add<PhysicsShapes::CCompound>("compound");
    ShapesFactory.add<PhysicsShapes::CCapsule>("capsule");
    ShapesFactory.add<PhysicsShapes::CSphere>("sphere");
    ShapesFactory.add<PhysicsShapes::CBox>("box");
}

class VHACDLogger : public VHACD::IVHACD::IUserLogger
{
public:
    void Log(const char* const msg) override
    {
        printf("[VHACD] %s\n", msg);
    }
};

class VHACDCallback : public VHACD::IVHACD::IUserCallback
{
public:
    void Update(const double overallProgress,
                const double stageProgress,
                const char* const stage,
                const char* operation) override
    {
        printf("[VHACD] %s (%s): %.1f%% (overall %.1f%%)\n",
               stage,
               operation,
               stageProgress * 100.0,
               overallProgress * 100.0);
    }
};

void PhysicsShapes::CConvex::BuildConvex()
{
    VHACD::IVHACD* vhacd = VHACD::CreateVHACD();
    VHACD::IVHACD::Parameters params;

    VHACDLogger   myLogger;
    VHACDCallback myCallback;

    Log::Instance() << "glm::vec3 sizeof is " << sizeof(glm::vec3) << Log::Endl;
    Log::Instance() << "glm::uvec3 sizeof is " << sizeof(glm::uvec3) << Log::Endl;

    const size_t vcount = Vertices.size();

    for (size_t i = 0; i < Triangles.size(); ++i)
    {
        const auto& t = Triangles[i];

        //Log::Instance() << "Triangle(" << t << ") as in (" << Vertices.at(t.x) << "); (" << Vertices.at(t.y) << "); (" << Vertices.at(t.z) << ")\n";

        if (t.x >= vcount || t.y >= vcount || t.z >= vcount)
        {
            printf("INVALID TRI INDEX at %zu: %u %u %u (vcount=%zu)\n",
                i, t.x, t.y, t.z, vcount);
        }

        if (t.x == t.y || t.y == t.z || t.x == t.z)
        {
            printf("DEGENERATE TRI (duplicate index) at %zu: %u %u %u\n",
                i, t.x, t.y, t.z);
        }
    }

    // 2) Check vertex memory layout
    static_assert(sizeof(glm::vec3) == sizeof(float) * 3,
                "Vec3 must be exactly 3 floats");
    static_assert(sizeof(glm::uvec3) == sizeof(uint32_t) * 3,
                "UVec3 must be exactly 3 floats");

                

    // 3) Check vertex values
    for (size_t i = 0; i < Vertices.size(); ++i)
    {
        const auto& v = Vertices[i];

        if(v.x >= 10000.0f || v.y >= 10000.0f || v.z >= 10000.0f)
        {
            Log::ErrInstance() << "TOO FAR AWAY " << v << Log::Endl;
        }

        if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z))
        {
            printf("INVALID VERTEX at %zu: %f %f %f\n", i, v.x, v.y, v.z);
        }
    }

    params.m_callback   = &myCallback;   // progress reporting
    params.m_logger     = &myLogger;     // diagnostic output
    params.m_taskRunner = nullptr;       // unless you have your own job system

    // --- Core behavior ---
    params.m_maxConvexHulls       = 1;        // EXACTLY one hull
    params.m_maxRecursionDepth   = 1;        // MUST be >= 1 to emit a hull
    params.m_resolution          = 100000;   // 50k–200k sweet spot
    params.m_minimumVolumePercentErrorAllowed = 1.0; // default is fine

    // --- Geometry handling ---
    params.m_fillMode            = VHACD::FillMode::SURFACE_ONLY;
    // Use FLOOD_FILL only if mesh is guaranteed watertight

    params.m_shrinkWrap          = true;
    // Now safe since your mesh is valid and closed-ish

    params.m_minEdgeLength       = 0;        // DO NOT block splitting
    params.m_findBestPlane       = false;    // experimental; keep off

    // --- Output hull quality ---
    params.m_maxNumVerticesPerCH = 256;      // Bullet stable limit
    params.m_asyncACD            = false;    // deterministic behavior

    Log::Instance() << "VHACD computing with " << Vertices.size() << " vertices and " << Triangles.size() << " triangles\n";

    bool succ = vhacd->Compute((float*)Vertices.data(), Vertices.size(), (std::uint32_t*)Triangles.data(), Triangles.size(), params);

    Log::Instance() << "Result is " << succ << Log::Endl;

    //TODO error handling: assert(hullCount == 1)
    //assert(vhacd->GetNConvexHulls() == 1);

    Vertices.clear();
    Triangles.clear();

    VHACD::IVHACD::ConvexHull hull;
    vhacd->GetConvexHull(0, hull);

    Log::Instance() << "vhacd->GetNConvexHulls() is " << vhacd->GetNConvexHulls() << Log::Endl;
    Log::Instance() << "hull.m_points.size() is " << hull.m_points.size() << Log::Endl;
    Log::Instance() << "hull.m_triangles.size() is " << hull.m_triangles.size() << Log::Endl;

    Vertices.reserve(hull.m_points.size());
    Triangles.reserve(hull.m_triangles.size());

    for (const VHACD::Vertex& v : hull.m_points)
    {
        Vertices.push_back({ v.mX, v.mY, v.mZ });
    }

    for (const VHACD::Triangle& t : hull.m_triangles)
    {
        Triangles.push_back({ t.mI0, t.mI1, t.mI2 });
    }

    Log::Instance() << "VHACD got " << Vertices.size() << " vertices and " << Triangles.size() << " triangles\n";

    vhacd->Release();
    //vhacd->Clean();
}

std::string CPhysicsEngine::GetType() const
{
    return {};
}

void CTriangleMesh::V_SetupLoadPipeline(CLoadPipeline& pipeline)
{
    pipeline.push_back //load image (async)
    (
        {
            [](std::shared_ptr<CLoadingContext> context) -> bool
            {
                auto thisResource = dynamic_cast<CTriangleMesh*>(context->CurrentResource);

                auto path = context->LoadPath;
                auto found_path = FileUtils::find_first_by_name(FileUtils::get_executable_path() / "resources" / "shapes", path.string());

                if(!found_path.has_value()) { return false; }

                CBinaryFile _bin;
                _bin.OpenRead(found_path.value());

                if(!_bin.IsOpen()) { return false; }

                auto filesize = _bin.Read<std::uint64_t>();

                Log::Instance() << "filesize is " << filesize << Log::Endl;

                auto vec = _bin.ReadDataVec(filesize);
                Log::Instance() << "allocd\n";
                _bin.Close();

                CBufferWrapper bin(vec);

                auto VerticesCount = bin.Read<std::uint64_t>();
                auto MaterialsCount = bin.Read<std::uint64_t>();

                Log::Instance() << "VerticesCount is " << VerticesCount << Log::Endl;
                Log::Instance() << "MaterialsCount is " << MaterialsCount << Log::Endl;

                for(std::uint64_t i = 0; i < VerticesCount; i++)
                {
                    auto v = bin.Read<glm::vec3>();
                    thisResource->Vertices.push_back(v);
                }

                for(std::uint64_t m = 0; m < MaterialsCount; m++)
                {
                    auto TrianglesCount = bin.Read<std::uint64_t>();
                    auto matName = bin.ReadLenString<std::uint8_t, std::string>();

                    for(std::uint64_t i = 0; i < TrianglesCount; i++)
                    {
                        CTriangle tri;
                        tri.Index = bin.Read<glm::u64vec3>();
                        tri.Material = 0; //TODO by matname
                        
                        thisResource->Triangles.push_back(std::move(tri));
                    }
                }
                return true;
            },
            true //async (is_async)
        }
    );
}

void LoadSingleConvexBinary(CBinaryFile& file, std::vector<glm::vec3>& vertices, std::vector<glm::uvec3>& triangles)
{
    auto VerticesCount = file.Read<std::uint32_t>();
    auto TrianglesCount = file.Read<std::uint32_t>();

    for(std::uint32_t i = 0; i < VerticesCount; i++)
    {
        vertices.push_back(file.Read<glm::vec3>());
    }

    for(std::uint32_t i = 0; i < TrianglesCount; i++)
    {
        triangles.push_back(file.Read<glm::uvec3>());
    }
}

void ApplyVhacdConfig(const std::filesystem::path& _config_path, VHACD::IVHACD::Parameters& params)
{
    using json = nlohmann::json;
    if(!_config_path.empty())
    {
        std::ifstream file;
        file.open(FileUtils::get_executable_path() / _config_path);

        if(file.is_open())
        {
            json data;
            bool valid = true;

            try
            {
                data = json::parse(file);
            }
            catch (const json::parse_error& e)
            {
                valid = false;
            }
            catch (const std::exception& e)
            {
                valid = false;
            }
            file.close();

            if(valid)
            {
                JsonToVHACD(data, params);
            }
        }
    }
}

void CCollision::CompileNativeConvex(const std::filesystem::path& _mesh_path, std::vector<std::filesystem::path> _configs_paths, bool vhacd)
{
    VHACD::IVHACD::Parameters params;

    if(vhacd)
    {
        for(auto& p : _configs_paths)
        {
            ApplyVhacdConfig(p, params);
        }
    }

    auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
    auto _trimesh = resman->GetOrCreate("trimesh", _mesh_path.string());

    _trimesh->Wait();
    auto trimesh = dynamic_cast<CTriangleMesh*>(_trimesh.get());

    struct WritePair
    {
        std::vector<glm::vec3> Vertices;
        std::vector<glm::uvec3> Triangles;

        glm::vec3 Center = glm::vec3(0.0f, 0.0f, 0.0f);
    };
    
    std::vector<glm::vec3> CurrentMeshVertices;
    std::vector<glm::uvec3> CurrentMeshTriangles;

    for(auto& v : trimesh->Vertices)
    {
        CurrentMeshVertices.push_back(v);
    }

    for(auto& tri : trimesh->Triangles)
    {
        CurrentMeshTriangles.push_back
        (
            {
                static_cast<glm::uvec3::value_type>(tri.Index.x),
                static_cast<glm::uvec3::value_type>(tri.Index.y),
                static_cast<glm::uvec3::value_type>(tri.Index.z)
            }
        );
    }

    std::vector<WritePair> ToWrite;
    if(!vhacd)
    {
        WritePair wr;

        wr.Vertices = std::move(CurrentMeshVertices);
        wr.Triangles = std::move(CurrentMeshTriangles);

        ToWrite.push_back(std::move(wr));
    }
    else
    {
        VHACD::IVHACD* vhacd = VHACD::CreateVHACD();
        CScopeExit scopeExit([vhacd]() { vhacd->Release(); });    

        VHACDLogger myLogger;
        VHACDCallback myCallback;

        params.m_callback = &myCallback;
        params.m_logger = &myLogger;
        params.m_taskRunner = nullptr;

        bool succ = vhacd->Compute((float*)CurrentMeshVertices.data(), CurrentMeshVertices.size(), (std::uint32_t*)CurrentMeshTriangles.data(), CurrentMeshTriangles.size(), params);

        uint32_t nconv = vhacd->GetNConvexHulls();
        for(uint32_t i = 0; i < nconv; i++)
        {
            VHACD::IVHACD::ConvexHull hull;
            vhacd->GetConvexHull(i, hull);

            std::vector<glm::vec3> vert;
            std::vector<glm::uvec3> tri;

            for (const VHACD::Vertex& v : hull.m_points)
            {
                vert.push_back({ v.mX, v.mY, v.mZ });
            }

            for (const VHACD::Triangle& t : hull.m_triangles)
            {
                tri.push_back({ t.mI0, t.mI1, t.mI2 });
            }

            WritePair wr;

            wr.Vertices = std::move(vert);
            wr.Triangles = std::move(tri);
            wr.Center = { hull.m_center.GetX(), hull.m_center.GetY(), hull.m_center.GetZ() };

            ToWrite.push_back(std::move(wr));
        }
    }

    auto outPath = FileUtils::get_executable_path() / "resources" / "convex" / (trimesh->Name + ".ecnv");
    CBinaryFile outFile;

    outFile.OpenWrite(outPath);
    outFile.Write<std::uint32_t>(ToWrite.size());

    for(std::uint32_t i = 0; i < ToWrite.size(); i++)
    {
        auto& wr = ToWrite.at(i);

        outFile.Write<glm::vec3>(wr.Center);

        outFile.Write<std::uint32_t>(wr.Vertices.size());
        outFile.Write<std::uint32_t>(wr.Triangles.size());

        for(auto& v : wr.Vertices) { outFile.Write<glm::vec3>(v); }
        for(auto& t : wr.Triangles) { outFile.Write<glm::uvec3>(t); }
    }

    outFile.Close();
    Log::Instance() << "Compiled convex with " << ToWrite.size() << " parts\n";
}

std::unique_ptr<PhysicsShapes::CBasic> CCollision::LoadNativeConvex(const nlohmann::json& _json)
{
    std::string path;
    path = GetFromJson<std::string>(_json, "mesh_path", "");
    if(path.empty()) { path = GetFromJson<std::string>(_json, "path", ""); }
    if(path.empty()) { path = GetFromJson<std::string>(_json, "convex_path", ""); }

    CBinaryFile file;
    auto found_path = FileUtils::find_first_by_name(FileUtils::get_executable_path() / "resources" / "convex", path);

    if(!found_path.has_value()) { return nullptr; }
    file.OpenRead(found_path.value());

    auto partsCount = file.Read<std::uint32_t>(); //1 - convex, >1 - compound

    std::unique_ptr<PhysicsShapes::CBasic> mainShape;
    std::string mainShapeType = (partsCount > 1) ? "compound" : "convex";

    auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
    mainShape = game->PhysicsEngine->ShapesFactory.create<std::unique_ptr<PhysicsShapes::CBasic>>(mainShapeType);

    for(std::uint32_t i = 0; i < partsCount; i++)
    {
        std::vector<glm::vec3> Vertices;
        std::vector<glm::uvec3> Triangles;

        glm::vec3 Center = glm::vec3(0.0f, 0.0f, 0.0f);

        Center = file.Read<glm::vec3>();
        LoadSingleConvexBinary(file, Vertices, Triangles);

        if(partsCount == 1)
        {
            auto mainConvex = dynamic_cast<PhysicsShapes::CConvex*>(mainShape.get());

            mainConvex->Vertices = std::move(Vertices);
            mainConvex->Triangles = std::move(Triangles);

            mainConvex->NeedVHACD = false;
            mainConvex->Build();
            return mainShape;
        }
        else
        {
            auto mainCompound = dynamic_cast<PhysicsShapes::CCompound*>(mainShape.get());

            std::shared_ptr<PhysicsShapes::CBasic> _conv = game->PhysicsEngine->ShapesFactory.create<std::shared_ptr<PhysicsShapes::CBasic>>("convex");
            auto conv = dynamic_cast<PhysicsShapes::CConvex*>(_conv.get());

            conv->Vertices = std::move(Vertices);
            conv->Triangles = std::move(Triangles);

            conv->NeedVHACD = false;
            conv->Build();
            CTransform trans;

            trans.SetPosition(Center);
            mainCompound->Childs.push_back({ trans, _conv });
        }
    }

    file.Close();
    mainShape->Build();
    return mainShape;
}

std::unique_ptr<PhysicsShapes::CBasic> CCollision::BuildConvexCompound(const nlohmann::json& _json)
{
    std::string path;
    path = GetFromJson<std::string>(_json, "mesh_path", "");

    bool withHoles = false;
    withHoles = GetFromJson<bool>(_json, "with_holes", false);

    auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
    auto _trimesh = resman->GetOrCreate("trimesh", path);

    _trimesh->Wait();
    auto trimesh = dynamic_cast<CTriangleMesh*>(_trimesh.get());

    std::vector<glm::vec3> Vertices;
    std::vector<glm::uvec3> Triangles;

    for(auto& v : trimesh->Vertices)
    {
        Vertices.push_back(v);
    }

    for(auto& tri : trimesh->Triangles)
    {
        Triangles.push_back
        (
            {
                static_cast<glm::uvec3::value_type>(tri.Index.x),
                static_cast<glm::uvec3::value_type>(tri.Index.y),
                static_cast<glm::uvec3::value_type>(tri.Index.z)
            }
        );
    }

    VHACD::IVHACD* vhacd = VHACD::CreateVHACD();
    CScopeExit scopeExit([vhacd]() { vhacd->Release(); });    

    VHACD::IVHACD::Parameters params;

    VHACDLogger myLogger;
    VHACDCallback myCallback;

    params.m_callback = &myCallback;
    params.m_logger = &myLogger;
    params.m_taskRunner = nullptr;

    if(withHoles)
    {
        params.m_maxConvexHulls = 128;
        params.m_resolution = 400000;
        params.m_minimumVolumePercentErrorAllowed = 2.0;
        params.m_maxRecursionDepth = 12;
        params.m_shrinkWrap = true;
        params.m_fillMode = VHACD::FillMode::SURFACE_ONLY;
        params.m_maxNumVerticesPerCH = 64;
        params.m_asyncACD = true;
        params.m_minEdgeLength = 2;
        params.m_findBestPlane = false;
    }
    else
    {
        params.m_maxConvexHulls = 64;
        params.m_resolution = 200000;
        params.m_minimumVolumePercentErrorAllowed = 1.0;
        params.m_maxRecursionDepth = 10;
        params.m_shrinkWrap = true;
        params.m_fillMode = VHACD::FillMode::FLOOD_FILL;
        params.m_maxNumVerticesPerCH = 64;
        params.m_asyncACD = true;
        params.m_minEdgeLength = 2;
        params.m_findBestPlane = false;
    }

    JsonToVHACD(_json, params);
    bool succ = vhacd->Compute((float*)Vertices.data(), Vertices.size(), (std::uint32_t*)Triangles.data(), Triangles.size(), params);

    auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
    uint32_t nconv = vhacd->GetNConvexHulls();

    std::unique_ptr<PhysicsShapes::CBasic> mainShape;
    std::string mainShapeType = "compound";

    if(nconv == 1)
    {
        mainShapeType = "convex";
    }
    mainShape = game->PhysicsEngine->ShapesFactory.create<std::unique_ptr<PhysicsShapes::CBasic>>(mainShapeType);

    for(uint32_t i = 0; i < nconv; i++)
    {
        VHACD::IVHACD::ConvexHull hull;
        vhacd->GetConvexHull(i, hull);

        std::vector<glm::vec3> vert;
        std::vector<glm::uvec3> tri;

        for (const VHACD::Vertex& v : hull.m_points)
        {
            vert.push_back({ v.mX, v.mY, v.mZ });
        }

        for (const VHACD::Triangle& t : hull.m_triangles)
        {
            tri.push_back({ t.mI0, t.mI1, t.mI2 });
        }

        if(nconv == 1)
        {
            auto mainConvex = dynamic_cast<PhysicsShapes::CConvex*>(mainShape.get());

            mainConvex->Vertices = std::move(vert);
            mainConvex->Triangles = std::move(tri);

            mainConvex->NeedVHACD = false;
            mainConvex->Build();
            return mainShape;
        }
        else
        {
            auto mainCompound = dynamic_cast<PhysicsShapes::CCompound*>(mainShape.get());

            std::shared_ptr<PhysicsShapes::CBasic> _conv = game->PhysicsEngine->ShapesFactory.create<std::shared_ptr<PhysicsShapes::CBasic>>("convex");
            auto conv = dynamic_cast<PhysicsShapes::CConvex*>(_conv.get());

            conv->Vertices = std::move(vert);
            conv->Triangles = std::move(tri);

            conv->NeedVHACD = false;
            conv->Build();
            CTransform trans;

            trans.SetPosition({ hull.m_center.GetX(), hull.m_center.GetY(), hull.m_center.GetZ() });
            mainCompound->Childs.push_back({ trans, _conv });
        }
    }

    mainShape->Build();
    return mainShape;
}

std::unique_ptr<PhysicsShapes::CBasic> CCollision::ParseShape(const nlohmann::json& _json, CTransformBase* _trans)
{
    using json = nlohmann::json;

    auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
    if(!game) { return nullptr; }
    
    if(_json.contains("type"))
    {
        if(_json.contains("transform") && _trans)
        {
            auto& transj = _json["transform"];
            CTransform trans;

            if(transj.is_object())
            {
                if(transj.contains("matrix"))
                {
                    auto& matrix = transj["matrix"];
                    if(matrix.is_array())
                    {
                        glm::mat4 matr;
                        std::vector<float> nums = matrix.get<std::vector<float>>();
                        for(size_t i = 0; i < nums.size() && i < (matr.length() * matr.length()); i++)
                        {
                            matr[i % 4][i / 4] = nums.at(i);
                        }
                        trans.SetFromMatrix(matr);
                    }
                }

                if(transj.contains("position"))
                {
                    auto& position = transj["position"];
                    if(position.is_array())
                    {
                        glm::vec3 pos;
                        std::vector<float> nums = position.get<std::vector<float>>();

                        for(size_t i = 0; i < nums.size() && i < pos.length(); i++)
                        {
                            pos[i] = nums.at(i);
                        }

                        trans.SetPosition(pos);
                    }
                }

                if(transj.contains("rotation"))
                {
                    auto& rotation = transj["rotation"];
                    if(rotation.is_array())
                    {
                        glm::quat quat;
                        std::vector<float> nums = rotation.get<std::vector<float>>();

                        for(size_t i = 0; i < nums.size() && i < quat.length(); i++)
                        {
                            quat[i] = nums.at(i);
                        }

                        trans.SetRotation(quat);
                    }
                }

                if(transj.contains("euler"))
                {
                    auto& euler = transj["euler"];
                    if(euler.is_array())
                    {
                        CAngles angles;
                        std::vector<float> nums = euler.get<std::vector<float>>();

                        for(size_t i = 0; i < nums.size() && i < angles.length(); i++)
                        {
                            angles[i] = CAngle::degrees(nums.at(i));
                        }

                        trans.GetEulerRotation().SetRotation(angles);
                    }
                }

                if(transj.contains("scale"))
                {
                    auto& scale = transj["scale"];
                    if(scale.is_array())
                    {
                        glm::vec3 scl;
                        std::vector<float> nums = scale.get<std::vector<float>>();

                        for(size_t i = 0; i < nums.size() && i < scl.length(); i++)
                        {
                            scl[i] = nums.at(i);
                        }

                        trans.SetScale(scl);
                    }
                }
            }
            _trans->SetPRS(trans.GetPRS());
        }

        const json& typej = _json["type"];

        float jsonMargin = -1.0f;
        if(_json.contains("margin"))
        {
            const json& marginj = _json["margin"];
            jsonMargin = marginj.get<float>();
        }

        PhysicsShapes::CBasic* shapeToSetParam = nullptr;
        CScopeExit setParams([&shapeToSetParam, jsonMargin]()
        {
            if(shapeToSetParam && jsonMargin != 1.0f)
            {
                shapeToSetParam->SetMargin(jsonMargin);
            }
        });

        std::string typestr;
        if(typej.is_string())
        {
            typestr = typej.get<std::string>();
        }
        if(typestr.empty()) { return nullptr; }

        if(typestr == "convex_compound")
        {
            auto ret = BuildConvexCompound(_json);
            shapeToSetParam = ret.get();
            return ret;
        }
        else if(typestr == "convex_native")
        {
            auto ret = LoadNativeConvex(_json);
            shapeToSetParam = ret.get();
            return ret;
        }

        std::unique_ptr<PhysicsShapes::CBasic> ret = game->PhysicsEngine->ShapesFactory.create<std::unique_ptr<PhysicsShapes::CBasic>>(typestr);
        if(!ret) { return nullptr; }

        ret->Load(_json);
        shapeToSetParam = ret.get();
        return ret;
    }
    return nullptr;
}

void CCollision::V_SetupLoadPipeline(CLoadPipeline& pipeline)
{
    pipeline.push_back //load image (async)
    (
        {
            [](std::shared_ptr<CLoadingContext> context) -> bool
            {
                auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
                if(!game) { return false; }

                using namespace nlohmann;
                auto thisResource = dynamic_cast<CCollision*>(context->CurrentResource);

                auto path = context->LoadPath;
                auto found_path = FileUtils::find_first_by_name(FileUtils::get_executable_path() / "resources" / "collision", path.string());

                if(!found_path.has_value()) { return false; }

                std::ifstream file(found_path.value());

                if (!file.is_open())
                {
                    return false;
                }

                json data;
                try
                {
                    data = json::parse(file);
                }
                catch (const json::parse_error& e)
                {
                    return false;
                }
                catch (const std::exception& e)
                {
                    return false;
                }
                file.close();

                Log::Instance() << "Parsing collision " << found_path.value().string() << Log::Endl;

                thisResource->Shape = ParseShape(data);
                if(!thisResource->Shape) { return false; }

                //return false;
                return true;
            },
            true //async (is_async)
        }
    );
}

CPhysicsWorld* CPhysicsEngine::GetWorld(int id)
{
    if(id < 0 || id >= m_Worlds.size()) { return nullptr; }
    return m_Worlds.at(id).get();
}

std::string PhysicsShapes::CBasic::GetType() const { return "basic"; }
std::string PhysicsShapes::CTriMesh::GetType() const { return "trimesh"; }
std::string PhysicsShapes::CConvex::GetType() const { return "convex"; }
std::string PhysicsShapes::CCompound::GetType() const { return "compound"; }
std::string PhysicsShapes::CCapsule::GetType() const { return "capsule"; }
std::string PhysicsShapes::CSphere::GetType() const { return "sphere"; }
std::string PhysicsShapes::CBox::GetType() const { return "box"; }

void CWorldUnit::OverridePersistent(CPhysicsWorld* _world)
{
    m_World = _world;
}

CPhysicsWorld* CWorldUnit::GetWorld() const
{
    return m_World;
}

bool CPhysicsEngine::V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
    table.set_function("getWorld", [this](int id, sol::this_state ts) -> sol::table
    {
        auto world = GetWorld(id);
        return world ? world->GetScriptTable(ts) : sol::lua_nil;
    });
    return true;
}

bool CPhysicsWorld::V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
    table.set_function("getGravity", [this]() -> glm::vec3 { return this->GetGravity(); });
    table.set_function("setGravity", [this](const glm::vec3& grav) { return this->SetGravity(grav); });

    table.set_function("createRigidBody", [this](const std::string& colname, float mass, sol::table _trans, sol::this_state ts) -> sol::object
    {
        if(colname.empty()) { return sol::lua_nil; }

        CTransform start_trans;
        if(_trans.valid())
        {
            start_trans = ScriptUtils::FromObject<CTransform>(_trans);
        }

        auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
        auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();

        auto _col = resman->GetOrCreate("collision", colname);
        auto col = std::dynamic_pointer_cast<CCollision>(_col);

        if(!col) { return sol::lua_nil; }

        auto ret = this->CreateRigidBody(col, start_trans, mass);
        return ret->GetScriptTable(ts);
    });

    table.set_function("createKinematicBody", [this](const std::string& colname, sol::table _trans, sol::this_state ts) -> sol::object
    {
        if(colname.empty()) { return sol::lua_nil; }

        CTransform start_trans;
        if(_trans.valid())
        {
            start_trans = ScriptUtils::FromObject<CTransform>(_trans);
        }

        auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
        auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();

        auto _col = resman->GetOrCreate("collision", colname);
        auto col = std::dynamic_pointer_cast<CCollision>(_col);

        if(!col) { return sol::lua_nil; }

        auto ret = this->CreateKinematicBody(col, start_trans);
        return ret->GetScriptTable(ts);
    });

    table.set_function("createGhostBody", [this](const std::string& colname, sol::table _trans, sol::this_state ts) -> sol::object
    {
        if(colname.empty()) { return sol::lua_nil; }

        CTransform start_trans;
        if(_trans.valid())
        {
            start_trans = ScriptUtils::FromObject<CTransform>(_trans);
        }

        auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
        auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();

        auto _col = resman->GetOrCreate("collision", colname);
        auto col = std::dynamic_pointer_cast<CCollision>(_col);

        if(!col) { return sol::lua_nil; }

        auto ret = this->CreateGhostBody(col, start_trans);
        return ret->GetScriptTable(ts);
    });
    return true;
}

bool CWorldUnit::V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
    return true;
}

EUnitType CRigidBody::GetType() const { return EUnitType::Rigid; }
EUnitType CKinematicBody::GetType() const { return EUnitType::Kinematic; }
EUnitType CGhostBody::GetType() const { return EUnitType::Ghost; }

LINK_RESOURCE_TO_CLASS(CTriangleMesh, trimesh);
LINK_RESOURCE_TO_CLASS(CCollision, collision);