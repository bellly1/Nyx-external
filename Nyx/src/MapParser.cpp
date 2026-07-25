#include "MapParser.hpp"

static std::string ReadName(Memory& mem, uintptr_t inst)
{
    if (!inst) return {};
    return mem.ReadStringPointer(inst + Offsets::Instance::Name);
}

static std::string ReadClassName(Memory& mem, uintptr_t inst)
{
    if (!inst) return {};
    const uintptr_t desc = mem.Read<uintptr_t>(inst + Offsets::Instance::ClassDescriptor);
    if (!desc) return {};
    return mem.ReadStringPointer(desc + Offsets::Instance::ClassName);
}

static std::vector<uintptr_t> ReadChildren(Memory& mem, uintptr_t inst)
{
    std::vector<uintptr_t> out;
    if (!inst) return out;

    const uintptr_t container = mem.Read<uintptr_t>(inst + Offsets::Instance::ChildrenStart);
    if (!container) return out;

    const uintptr_t start = mem.Read<uintptr_t>(container);
    const uintptr_t end = mem.Read<uintptr_t>(container + Offsets::Instance::ChildrenEnd);
    if (!start || !end || end < start) return out;

    const size_t count = (end - start) / 0x10;
    if (!count || count > 2048) return out;

    out.reserve(count);
    for (uintptr_t n = start; n < end; n += 0x10)
    {
        const uintptr_t child = mem.Read<uintptr_t>(n);
        if (child) out.push_back(child);
    }
    return out;
}

static bool HasHumanoidChild(Memory& mem, uintptr_t model)
{
    for (const uintptr_t c : ReadChildren(mem, model))
    {
        if (ReadClassName(mem, c) == "Humanoid")
            return true;
        const std::string cn = ReadClassName(mem, c);
        if (cn == "Folder" || cn == "Model")
        {
            for (const uintptr_t c2 : ReadChildren(mem, c))
                if (ReadClassName(mem, c2) == "Humanoid")
                    return true;
        }
    }
    return false;
}

void MapParser::ClassifyShape(const std::string& className, const std::string& nameLower,
    const Vector3& size, uintptr_t instance, ParsedPart& out) const
{
    if (className == "WedgePart") { out.shape = PartShape::wedge; return; }
    if (className == "CornerWedgePart") { out.shape = PartShape::corner_wedge; return; }
    if (className == "TrussPart") { out.shape = PartShape::truss; return; }

    bool foundShape = false;
    {
        const std::vector<uintptr_t> children = ReadChildren(const_cast<Memory&>(m_memory), instance);
        for (uintptr_t child : children)
        {
            if (ReadClassName(m_memory, child) == "SpecialMesh")
            {

                const int meshType = m_memory.Read<int>(child + 0x1b0);
                if (meshType == 3) { out.shape = PartShape::sphere; foundShape = true; break; }
                if (meshType == 4) { out.shape = PartShape::cylinder; foundShape = true; break; }
                if (meshType == 2) { out.shape = PartShape::wedge; foundShape = true; break; }
            }
        }
    }

    if (foundShape) return;

    auto hasHint = [](const std::string& hay, const char* const* needles, int count) -> bool {
        for (int i = 0; i < count; ++i)
            if (hay.find(needles[i]) != std::string::npos) return true;
        return false;
    };

    const char* roundHints[] = { "sphere","ball","orb","head","round","bullet","circle","pill","cap","dome" };
    const char* cylHints[] = { "cylinder","pipe","tube","pole","column","wire","cable","barrel","disc","coin" };

    const bool hasRound = hasHint(nameLower, roundHints, 10);
    const bool hasCyl = hasHint(nameLower, cylHints, 10);

    const float dxY = std::abs(size.x - size.y);
    const float dxZ = std::abs(size.x - size.z);
    const float dyZ = std::abs(size.y - size.z);
    constexpr float kTol = 0.02f;

    const bool isCube = (dxY < kTol && dxZ < kTol);
    const bool isCylX = (dyZ < kTol);
    const bool isCylY = (dxZ < kTol);
    const bool isCylZ = (dxY < kTol);

    if (hasRound && isCube) { out.shape = PartShape::sphere; return; }
    if (hasCyl && (isCylX || isCylY || isCylZ)) { out.shape = PartShape::cylinder; return; }

    if (className == "MeshPart")
    {
        if (isCube) { out.shape = PartShape::sphere; return; }
        if (isCylX || isCylY || isCylZ) { out.shape = PartShape::cylinder; return; }
        out.shape = PartShape::mesh;
        return;
    }

    if (isCube)
    {
        out.shape = (size.x < 10.0f) ? PartShape::sphere : PartShape::box;
        return;
    }
    if (isCylX || isCylY || isCylZ) { out.shape = PartShape::cylinder; return; }
    out.shape = PartShape::box;
}

void MapParser::Scan(uintptr_t workspace)
{
    std::vector<ParsedPart> newParts;
    newParts.reserve(65000);

    if (!workspace) return;

    auto isServiceSkip = [](const std::string& cn) -> bool {
        return cn == "Players" || cn == "Debris" || cn == "Lighting"
            || cn == "Teams" || cn == "SoundService" || cn == "StarterGui"
            || cn == "StarterPack" || cn == "StarterPlayer"
            || cn == "Camera" || cn == "Terrain"
            || cn == "LocalScript" || cn == "Script" || cn == "ModuleScript"
            || cn == "Chat" || cn == "TextChatService";
    };

    auto isPartClass = [](const std::string& cn) -> bool {
        return cn == "Part" || cn == "MeshPart" || cn == "CornerWedgePart"
            || cn == "WedgePart" || cn == "TrussPart" || cn == "UnionOperation"
            || cn == "SpawnLocation" || cn == "Seat" || cn == "VehicleSeat";
    };

    struct Node { uintptr_t inst; int depth; };
    std::vector<Node> stack;
    stack.reserve(4096);
    stack.push_back({ workspace, 0 });

    constexpr size_t kMaxParts = 65000;
    constexpr int kMaxDepth = 48;

    while (!stack.empty() && newParts.size() < kMaxParts)
    {
        const Node n = stack.back();
        stack.pop_back();
        if (!n.inst || n.depth > kMaxDepth) continue;

        const std::string cn = ReadClassName(m_memory, n.inst);

        if (n.depth > 0 && isServiceSkip(cn))
            continue;

        if (cn == "Model")
        {
            if (HasHumanoidChild(m_memory, n.inst))
                continue;
        }
        if (cn == "Accessory" || cn == "Hat" || cn == "Tool" || cn == "HopperBin")
            continue;

        if (isPartClass(cn))
        {
            const uintptr_t prim = m_memory.Read<uintptr_t>(n.inst + Offsets::BasePart::Primitive);
            if (!prim) goto expand;

            // Only solid, collidable parts count as walls for vis-check
            if (Offsets::Primitive::Flags)
            {
                const uint8_t flags = m_memory.Read<uint8_t>(prim + Offsets::Primitive::Flags);
                auto flagOn = [&](uintptr_t bit) -> bool {
                    if (!bit) return true; // unknown → don't skip
                    if (bit <= 7)
                        return (flags & static_cast<uint8_t>(1u << bit)) != 0;
                    return (flags & static_cast<uint8_t>(bit & 0xFFu)) != 0;
                };
                const uintptr_t cc = Offsets::PrimitiveFlags::CanCollide;
                const uintptr_t cq = Offsets::PrimitiveFlags::CanQuery;
                if (cc && !flagOn(cc)) goto expand; // non-colliding = not a wall
                if (cq && !flagOn(cq)) goto expand; // not raycastable
            }

            // Fully transparent parts (glass panes with high transparency, etc.)
            if (Offsets::BasePart::Transparency)
            {
                const float tr = m_memory.Read<float>(n.inst + Offsets::BasePart::Transparency);
                if (tr >= 0.85f) goto expand;
            }

            {
                Vector3 size = m_memory.Read<Vector3>(prim + Offsets::Primitive::Size);
                // Ignore tiny props / particles / debris
                if (size.x < 0.45f && size.y < 0.45f && size.z < 0.45f) goto expand;
                // Ignore giant skybox-style slabs that drown raycasts
                if (size.x > 2000.f || size.y > 2000.f || size.z > 2000.f) goto expand;
                // Ultra-thin non-vertical sheets often are decals/floors of zero thickness noise
                const float minAxis = (std::min)({ size.x, size.y, size.z });
                const float maxAxis = (std::max)({ size.x, size.y, size.z });
                if (minAxis < 0.12f && maxAxis < 2.0f) goto expand;

                Vector3 pos = m_memory.Read<Vector3>(prim + Offsets::Primitive::Position);
                if (pos.LengthSquared() > 1e12f) goto expand;

                const std::string nameLower = [&]() {
                    std::string name = ReadName(m_memory, n.inst);
                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                    return name;
                }();

                // Name-based: skip FX / glass / water / non-geometry
                auto nameHit = [&](const char* s) {
                    return nameLower.find(s) != std::string::npos;
                };
                if (nameHit("glass") || nameHit("window") || nameHit("water")
                    || nameHit("smoke") || nameHit("fog") || nameHit("cloud")
                    || nameHit("particle") || nameHit("effect") || nameHit("beam")
                    || nameHit("trail") || nameHit("spark") || nameHit("fire")
                    || nameHit("light") || nameHit("neon") || nameHit("holo")
                    || nameHit("trigger") || nameHit("hitbox") || nameHit("barrier")
                    || nameHit("zone") || nameHit("kill") || nameHit("spawn")
                    || nameHit("decal") || nameHit("leaf") || nameHit("grass")
                    || nameHit("flower") || nameHit("bush") || nameHit("plant"))
                    goto expand;

                ParsedPart parsed{};
                parsed.position = pos;
                parsed.size = size;

                if (Offsets::Primitive::Rotation)
                    parsed.rotation = m_memory.Read<Matrix3>(prim + Offsets::Primitive::Rotation);

                ClassifyShape(cn, nameLower, size, n.inst, parsed);

                // Prefer tight OBB radius (half diagonal) for broadphase
                const float hx = size.x * 0.5f, hy = size.y * 0.5f, hz = size.z * 0.5f;
                parsed.radius = std::sqrt(hx * hx + hy * hy + hz * hz) + 0.05f;
                parsed.radius_sq = parsed.radius * parsed.radius;

                newParts.push_back(parsed);
            }

        expand:;
        }

        for (const uintptr_t child : ReadChildren(m_memory, n.inst))
        {
            if (newParts.size() >= kMaxParts) break;
            stack.push_back({ child, n.depth + 1 });
        }
    }

    {
        std::unique_lock lock(m_mutex);
        m_parts = std::move(newParts);
    }
}

bool MapParser::IsVisible(const Vector3& start, const Vector3& end,
    uintptr_t ignoreModel, bool includePlayers, const PlayerFrame* players) const
{
    std::shared_lock lock(m_mutex);

    if (m_parts.empty() && !includePlayers) return true;

    Vector3 dir = end - start;
    float maxDist = dir.Length();
    if (maxDist < 0.001f) return true;

    Vector3 unitDir = dir * (1.f / maxDist);
    // Ignore hits extremely close to either endpoint (surface skin / self)
    constexpr float kNearEps = 0.08f;
    constexpr float kFarEps = 0.12f;

    auto intersects = [&](const ParsedPart& part) -> bool {

        Vector3 toCenter = part.position - start;
        float projection = Dot(toCenter, unitDir);
        if (projection < -1.0f) return false;
        if (projection > maxDist + 1.0f) return false;

        float distSq = Dot(toCenter, toCenter) - (projection * projection);
        if (distSq > part.radius_sq) return false;

        if (part.shape == PartShape::sphere)
        {
            float r = part.size.x * 0.5f;
            float rSq = r * r;
            Vector3 oc = start - part.position;
            float b = Dot(oc, unitDir);
            float c = Dot(oc, oc) - rSq;
            float disc = b * b - c;
            if (disc < 0) return false;
            float t = -b - std::sqrt(disc);
            if (t < 0) t = -b + std::sqrt(disc);
            return t >= 0 && t <= maxDist;
        }
        else if (part.shape == PartShape::cylinder)
        {
            Vector3 localOrigin = start - part.position;
            Vector3 ro = {
                localOrigin.x * part.rotation.m[0] + localOrigin.y * part.rotation.m[3] + localOrigin.z * part.rotation.m[6],
                localOrigin.x * part.rotation.m[1] + localOrigin.y * part.rotation.m[4] + localOrigin.z * part.rotation.m[7],
                localOrigin.x * part.rotation.m[2] + localOrigin.y * part.rotation.m[5] + localOrigin.z * part.rotation.m[8]
            };
            Vector3 rd = {
                unitDir.x * part.rotation.m[0] + unitDir.y * part.rotation.m[3] + unitDir.z * part.rotation.m[6],
                unitDir.x * part.rotation.m[1] + unitDir.y * part.rotation.m[4] + unitDir.z * part.rotation.m[7],
                unitDir.x * part.rotation.m[2] + unitDir.y * part.rotation.m[5] + unitDir.z * part.rotation.m[8]
            };

            float r, hHalf;
            int axis = 1;
            constexpr float kTol = 0.02f;

            if (std::abs(part.size.x - part.size.z) < kTol) { r = part.size.x * 0.5f; hHalf = part.size.y * 0.5f; axis = 1; }
            else if (std::abs(part.size.x - part.size.y) < kTol) { r = part.size.x * 0.5f; hHalf = part.size.z * 0.5f; axis = 2; }
            else { r = part.size.y * 0.5f; hHalf = part.size.x * 0.5f; axis = 0; }

            float rSq = r * r;
            float a, bv, cv;
            float roAxis, rdAxis;

            if (axis == 1) { a = rd.x * rd.x + rd.z * rd.z; bv = 2.f * (ro.x * rd.x + ro.z * rd.z); cv = ro.x * ro.x + ro.z * ro.z - rSq; roAxis = ro.y; rdAxis = rd.y; }
            else if (axis == 2) { a = rd.x * rd.x + rd.y * rd.y; bv = 2.f * (ro.x * rd.x + ro.y * rd.y); cv = ro.x * ro.x + ro.y * ro.y - rSq; roAxis = ro.z; rdAxis = rd.z; }
            else { a = rd.y * rd.y + rd.z * rd.z; bv = 2.f * (ro.y * rd.y + ro.z * rd.z); cv = ro.y * ro.y + ro.z * ro.z - rSq; roAxis = ro.x; rdAxis = rd.x; }

            if (std::abs(a) > 0.0001f)
            {
                float disc = bv * bv - 4.f * a * cv;
                if (disc >= 0)
                {
                    float sd = std::sqrt(disc);
                    float t0 = (-bv - sd) / (2.f * a);
                    float t1 = (-bv + sd) / (2.f * a);
                    for (float t : { t0, t1 })
                    {
                        if (t >= 0 && t <= maxDist)
                        {
                            float pa = roAxis + t * rdAxis;
                            if (std::abs(pa) <= hHalf) return true;
                        }
                    }
                }
            }

            if (std::abs(rdAxis) > 0.0001f)
            {
                auto capHit = [&](float tCap) -> bool {
                    if (tCap < 0 || tCap > maxDist) return false;
                    Vector3 p = ro + rd * tCap;
                    float dsq;
                    if (axis == 1) dsq = p.x * p.x + p.z * p.z;
                    else if (axis == 2) dsq = p.x * p.x + p.y * p.y;
                    else dsq = p.y * p.y + p.z * p.z;
                    return dsq <= rSq;
                };
                if (capHit((hHalf - roAxis) / rdAxis)) return true;
                if (capHit((-hHalf - roAxis) / rdAxis)) return true;
            }
            return false;
        }
        else if (part.shape == PartShape::wedge)
        {
            Vector3 localOrigin = start - part.position;
            Vector3 ro = {
                Dot(localOrigin, { part.rotation.m[0], part.rotation.m[3], part.rotation.m[6] }),
                Dot(localOrigin, { part.rotation.m[1], part.rotation.m[4], part.rotation.m[7] }),
                Dot(localOrigin, { part.rotation.m[2], part.rotation.m[5], part.rotation.m[8] })
            };
            Vector3 rd = {
                Dot(unitDir, { part.rotation.m[0], part.rotation.m[3], part.rotation.m[6] }),
                Dot(unitDir, { part.rotation.m[1], part.rotation.m[4], part.rotation.m[7] }),
                Dot(unitDir, { part.rotation.m[2], part.rotation.m[5], part.rotation.m[8] })
            };

            Vector3 hs = part.size * 0.5f;
            float tmin = -1e30f, tmax = 1e30f;

            auto clip = [&](float dist, float d) {
                if (std::abs(d) < 1e-6f) { if (dist > 0) tmax = -1.f; return; }
                float t = -dist / d;
                if (d > 0) tmin = (std::max)(tmin, t); else tmax = (std::min)(tmax, t);
            };

            clip(ro.x + hs.x, rd.x);
            clip(hs.x - ro.x, -rd.x);
            clip(ro.y + hs.y, rd.y);
            clip(hs.z - ro.z, -rd.z);

            float ny = part.size.z, nz = -part.size.y;
            clip(ro.y * ny + ro.z * nz, rd.y * ny + rd.z * nz);

            if (tmax < 0 || tmin > tmax) return false;
            return tmin <= maxDist;
        }
        else if (part.shape == PartShape::corner_wedge)
        {
            Vector3 localOrigin = start - part.position;
            Vector3 ro = {
                Dot(localOrigin, { part.rotation.m[0], part.rotation.m[3], part.rotation.m[6] }),
                Dot(localOrigin, { part.rotation.m[1], part.rotation.m[4], part.rotation.m[7] }),
                Dot(localOrigin, { part.rotation.m[2], part.rotation.m[5], part.rotation.m[8] })
            };
            Vector3 rd = {
                Dot(unitDir, { part.rotation.m[0], part.rotation.m[3], part.rotation.m[6] }),
                Dot(unitDir, { part.rotation.m[1], part.rotation.m[4], part.rotation.m[7] }),
                Dot(unitDir, { part.rotation.m[2], part.rotation.m[5], part.rotation.m[8] })
            };

            Vector3 hs = part.size * 0.5f;
            float tmin = -1e30f, tmax = 1e30f;

            auto clip = [&](Vector3 normal, float distFromOrigin) {
                float d = Dot(rd, normal);
                float o = Dot(ro, normal) - distFromOrigin;
                if (std::abs(d) < 1e-6f) { if (o > 0) tmax = -1.f; return; }
                float t = -o / d;
                if (d > 0) tmin = (std::max)(tmin, t); else tmax = (std::min)(tmax, t);
            };

            clip({ 0, -1, 0 }, -hs.y);
            clip({ 0, 0, 1 }, hs.z);
            clip({ -1, 0, 0 }, -hs.x);

            Vector3 n4 = { 0, -part.size.z, part.size.y };
            float n4m = n4.Length();
            clip({ n4.x / n4m, n4.y / n4m, n4.z / n4m }, Dot(n4, { -hs.x, -hs.y, -hs.z }) / n4m);

            Vector3 n5 = { -part.size.y, -part.size.x, 0.f };
            float n5m = n5.Length();
            clip({ n5.x / n5m, n5.y / n5m, n5.z / n5m }, Dot(n5, { hs.x, -hs.y, hs.z }) / n5m);

            if (tmax < 0 || tmin > tmax) return false;
            return tmin <= maxDist;
        }
        else
        {
            // Oriented box (default wall brick) — proper slab raycast
            Vector3 localOrigin = start - part.position;
            Vector3 ro = {
                Dot(localOrigin, { part.rotation.m[0], part.rotation.m[3], part.rotation.m[6] }),
                Dot(localOrigin, { part.rotation.m[1], part.rotation.m[4], part.rotation.m[7] }),
                Dot(localOrigin, { part.rotation.m[2], part.rotation.m[5], part.rotation.m[8] })
            };
            Vector3 rd = {
                Dot(unitDir, { part.rotation.m[0], part.rotation.m[3], part.rotation.m[6] }),
                Dot(unitDir, { part.rotation.m[1], part.rotation.m[4], part.rotation.m[7] }),
                Dot(unitDir, { part.rotation.m[2], part.rotation.m[5], part.rotation.m[8] })
            };

            Vector3 hs = part.size * 0.5f;
            // Slight inflate so thin walls still register
            hs.x += 0.02f; hs.y += 0.02f; hs.z += 0.02f;

            auto safeInv = [](float v) -> float {
                return (std::abs(v) < 1e-8f) ? (v >= 0.f ? 1e8f : -1e8f) : (1.f / v);
            };
            const float ix = safeInv(rd.x), iy = safeInv(rd.y), iz = safeInv(rd.z);

            float t1 = (-hs.x - ro.x) * ix;
            float t2 = (hs.x - ro.x) * ix;
            float t3 = (-hs.y - ro.y) * iy;
            float t4 = (hs.y - ro.y) * iy;
            float t5 = (-hs.z - ro.z) * iz;
            float t6 = (hs.z - ro.z) * iz;

            float tmin = (std::max)({ (std::min)(t1, t2), (std::min)(t3, t4), (std::min)(t5, t6) });
            float tmax = (std::min)({ (std::max)(t1, t2), (std::max)(t3, t4), (std::max)(t5, t6) });

            if (tmax < kNearEps || tmin > tmax) return false;
            // Hit must land on the segment between start and end (not past target)
            const float hitT = (tmin >= kNearEps) ? tmin : tmax;
            return hitT >= kNearEps && hitT <= (maxDist - kFarEps);
        }
    };

    for (const auto& part : m_parts)
    {
        // Skip absurdly small broadphase misses already handled
        if (intersects(part))
            return false; // blocked by a solid wall part
    }

    if (includePlayers && players)
    {
        for (const auto& player : players->entities)
        {
            if (player.address == ignoreModel) continue;
            if (player.health <= 0) continue;

            Vector3 toPlayer = player.rootPos - start;
            float projection = Dot(toPlayer, unitDir);
            if (projection < -5.0f || projection > maxDist + 10.0f) continue;

            float distSq = Dot(toPlayer, toPlayer) - (projection * projection);
            if (distSq > 2500.0f) continue;

            ParsedPart temp{};
            temp.position = player.rootPos;
            temp.size = { 2.f, 5.f, 2.f };
            temp.shape = PartShape::box;
            float ps = 2.f * 2.f + 5.f * 5.f + 2.f * 2.f;
            temp.radius_sq = ps * 0.25f;

            if (intersects(temp))
                return false;
        }
    }

    return true;
}

void MapParser::Clear()
{
    std::unique_lock lock(m_mutex);
    m_parts.clear();
}

