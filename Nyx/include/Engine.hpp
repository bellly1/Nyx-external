#pragma once

#include "Memory.hpp"
#include "GameData.hpp"
#include "DoubleBuffer.hpp"
#include "Config.hpp"
#include "Timer.hpp"
#include "MapParser.hpp"

#include <atomic>
#include <thread>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <mutex>

class Engine
{
public:
    explicit Engine(Memory& memory);
    ~Engine();

    void Start();
    void Stop();

    std::shared_ptr<const GlobalsData> Globals() const { return m_globals.Read(); }
    std::shared_ptr<const ActorCluster> Actors() const { return m_actors.Read(); }
    std::shared_ptr<const PlayerFrame> Players() const { return m_players.Read(); }
    std::shared_ptr<const CameraData> Camera() const { return m_camera.Read(); }
    std::shared_ptr<const AimLockData> AimLock() const { return m_aimLock.Read(); }
    std::shared_ptr<const OcclusionFrame> Occlusion() const { return m_occlusion.Read(); }
    std::shared_ptr<const TracerFrame> Tracers() const { return m_tracers.Read(); }

    void PushBulletTracer(const Vector3& from, const Vector3& to, float duration);

    EngineTimings& Timings() { return m_timings; }
    const EngineTimings& Timings() const { return m_timings; }

    ConfigStore& Settings() { return m_config; }
    const ConfigStore& Settings() const { return m_config; }

    // Instant apply while World menu is open (scripts may reset Lighting)
    void ForceWorldLighting();

    std::atomic<bool> running{ true };

    std::atomic<uint64_t> hitEventSeq{ 0 };
    std::atomic<int> hitEventKind{ 0 };
    void SignalHit() { hitEventKind.store(0, std::memory_order_relaxed); hitEventSeq.fetch_add(1, std::memory_order_relaxed); }
    void SignalLock() { hitEventKind.store(1, std::memory_order_relaxed); hitEventSeq.fetch_add(1, std::memory_order_relaxed); }

    // Used by overlay for Vis Check ESP colors
    bool HasLineOfSight(const CameraData& cam, const Vector3& target,
        const OcclusionFrame* occ, uintptr_t skipPlayer) const;
    bool IsEntityVisibleEsp(const CameraData& cam, const EntityData& e,
        const OcclusionFrame* occ) const;

private:
    void GlobalsThread();
    void ActorsThread();
    void PlayersThread();
    void CameraThread();
    void AimThread();
    void MovementThread();
    void WorldThread();
    void OcclusionThread();
    void TriggerbotThread();

    void SleepBudget(double targetMs, double elapsedMs) const;
    void ApplyVelocityMovement(const Config& cfg, const CameraData& cam, uintptr_t rootPart) const;
    void ApplyWorldLighting(const Config& cfg, uintptr_t lighting);
    uintptr_t ResolveLighting() const;

    void ClickMouse(float releaseMs) const;
    void MoveMouseRelative(int dx, int dy) const;
    void MoveMouseTowardScreen(HWND hwnd, float targetClientX, float targetClientY, float strength) const;

    std::string ReadName(uintptr_t instance) const;
    std::string ReadClassName(uintptr_t instance) const;
    std::vector<uintptr_t> ReadChildren(uintptr_t instance) const;
    Vector3 ReadPartPos(uintptr_t part) const;
    Vector3 ReadPartSize(uintptr_t part) const;
    BoneId BoneFromName(const std::string& name, bool& isR15) const;
    void FillBones(EntityData& entity, const std::vector<uintptr_t>& children) const;
    void FillBonesDeep(EntityData& entity, uintptr_t root, int maxDepth = 22) const;
    void CollectChamParts(EntityData& entity, uintptr_t root, int maxDepth = 18) const;
    bool IsOverkillPlace(int64_t placeId, int64_t gameId = 0) const;
    bool DetectOverkillWorkspace(uintptr_t workspace) const;
    uintptr_t FindChildByName(uintptr_t parent, const char* name) const;
    uintptr_t FindModelFromHumanoid(uintptr_t humanoid) const;
    uintptr_t FindLocalModelFromCamera(uintptr_t workspace) const;
    void DiscoverOverkillPlayers(uintptr_t workspace, std::vector<uintptr_t>& outModels) const;
    void DiscoverOnyxStandalone(uintptr_t workspace, std::vector<uintptr_t>& outModels,
        const std::unordered_set<uintptr_t>& already) const;
    void MatchOnyxHitboxes(uintptr_t workspace, std::vector<EntityData>& entities) const;
    void RefreshOverkillEntity(EntityData& e, uintptr_t model);
    void AppendWorkspaceNpcs(uintptr_t workspace, PlayerFrame& frame) const;
    bool FillEntityFromModel(EntityData& e, uintptr_t model) const;

    struct OverkillRigCache
    {
        uintptr_t humanoid = 0;
        uintptr_t rootPart = 0;
        uintptr_t boneParts[kBoneCount]{};
        bool barePart = false;
        bool ready = false;
        std::chrono::steady_clock::time_point lastStructure{};
    };
    std::unordered_map<uintptr_t, OverkillRigCache> m_okCache;
    std::chrono::steady_clock::time_point m_lastOnyxMatch{};
    Vector3 AimPoint(const EntityData& e, AimBone bone) const;
    Vector3 ClosestBoneToCursor(const EntityData& e, const Vector2& cursor, const Matrix4& view, float screenW, float screenH) const;
    Vector2 CursorInGame(const GlobalsData& g, const CameraData& cam) const;
    Vector2 CursorInViewport(const GlobalsData& g, const Vector2& viewport) const;
    HWND FindRobloxWindow() const;
    uintptr_t ResolveCharacter(uintptr_t player, const std::string& playerName, uintptr_t workspace) const;

    bool GetViewData(Matrix4& view, Vector2& viewport) const;
    void ApplySilentAim(const GlobalsData& g, const Vector3& world) const;

    Memory& m_memory;
    ConfigStore m_config;
    EngineTimings m_timings{};
    MapParser m_mapParser;

    DoubleBuffer<GlobalsData> m_globals;
    DoubleBuffer<ActorCluster> m_actors;
    DoubleBuffer<PlayerFrame> m_players;
    DoubleBuffer<CameraData> m_camera;
    DoubleBuffer<AimLockData> m_aimLock;
    DoubleBuffer<OcclusionFrame> m_occlusion;
    DoubleBuffer<TracerFrame> m_tracers;
    mutable std::mutex m_tracerMutex;
    std::vector<BulletTracer> m_tracerLive;
    void TickTracers(float dt);

    std::thread m_globalsThread;
    std::thread m_actorsThread;
    std::thread m_playersThread;
    std::thread m_cameraThread;
    std::thread m_aimThread;
    std::thread m_movementThread;
    std::thread m_worldThread;
    std::thread m_occlusionThread;
    std::thread m_triggerbotThread;

    std::atomic<uint64_t> m_seqGlobals{ 0 };
    std::atomic<uint64_t> m_seqActors{ 0 };
    std::atomic<uint64_t> m_seqPlayers{ 0 };
    std::atomic<uint64_t> m_seqCamera{ 0 };
    std::atomic<uint64_t> m_seqOcclusion{ 0 };
};

