#pragma once

#include "Math.hpp"
#include "Bones.hpp"
#include "Timer.hpp"

#include <array>
#include <string>
#include <vector>
#include <cstdint>

struct GlobalsData
{
    uintptr_t moduleBase = 0;
    uintptr_t fakeDataModel = 0;
    uintptr_t dataModel = 0;
    uintptr_t workspace = 0;
    uintptr_t players = 0;
    uintptr_t localPlayer = 0;
    uintptr_t currentCamera = 0;
    uintptr_t mouseService = 0;
    uintptr_t lighting = 0;
    uintptr_t robloxHwnd = 0;
    int clientWidth = 0;
    int clientHeight = 0;
    int clientLeft = 0;
    int clientTop = 0;
    int64_t placeId = 0;
    int64_t gameId = 0;
    bool gameLoaded = false;
    bool overkillMode = false;
    bool valid = false;
    uint64_t sequence = 0;
};

struct ActorCluster
{
    std::vector<uintptr_t> actors;
    uintptr_t localPlayer = 0;
    uintptr_t playersService = 0;
    bool valid = false;
    uint64_t sequence = 0;
};

enum class ChamKind : int { Body = 0, Hand = 1, Gun = 2, Glass = 3 };
struct ChamPart
{
    Vector3 pos{};
    Vector3 half{};
    ChamKind kind = ChamKind::Body;
    bool valid = false;
};

inline constexpr int kMaxChamParts = 40;

struct EntityData
{
    uintptr_t address = 0;
    uintptr_t character = 0;
    uintptr_t humanoid = 0;
    uintptr_t rootPart = 0;
    std::string name;
    std::string displayName;
    float health = 0.f;
    float maxHealth = 0.f;
    Vector3 rootPos{};
    Vector3 velocity{};
    uintptr_t team = 0;
    std::string equipped;
    std::array<BoneJoint, kBoneCount> bones{};
    int validBones = 0;
    bool isLocal = false;
    bool isNpc = false;
    bool hasCharacter = false;
    bool isR15 = false;

    std::array<ChamPart, kMaxChamParts> chams{};
    int chamCount = 0;
};

struct PlayerFrame
{
    std::vector<EntityData> entities;
    bool valid = false;
    uint64_t sequence = 0;
};

struct CameraData
{
    Matrix4 viewMatrix{};
    Matrix3 rotation{};
    Vector3 position{};
    float dimensionsX = 0.f;
    float dimensionsY = 0.f;
    float fov = 70.f;
    uintptr_t visualEngine = 0;
    uintptr_t camera = 0;
    bool valid = false;
    uint64_t sequence = 0;
};

struct BulletTracer
{
    Vector3 from{};
    Vector3 to{};
    float life = 0.f;
    float maxLife = 0.4f;
};

struct TracerFrame
{
    std::vector<BulletTracer> items;
    uint64_t sequence = 0;
};

enum class PartShape : int
{
    box = 0,
    sphere,
    cylinder,
    wedge,
    corner_wedge,
    truss,
    mesh
};

struct ParsedPart
{
    Vector3 position{};
    Vector3 size{};
    Matrix3 rotation{};
    PartShape shape = PartShape::box;
    float radius = 0.f;
    float radius_sq = 0.f;
};

struct OcclusionFrame
{
    std::vector<ParsedPart> parts;
    bool valid = false;
    uint64_t sequence = 0;
};

struct AimLockData
{
    bool locked = false;
    bool active = false;
    bool silent = false;
    bool useCenter = false;
    float fovRadius = 0.f;
    Vector2 cursorScreen{};
    Vector2 targetScreen{};
    Vector3 targetWorld{};
    uintptr_t targetPlayer = 0;
    uint64_t sequence = 0;
};

struct EngineTimings
{
    ThreadStats globals;
    ThreadStats actors;
    ThreadStats players;
    ThreadStats camera;
    ThreadStats aim;
    ThreadStats render;
};

