#pragma once

#include "Math.hpp"
#include "Memory.hpp"
#include "GameData.hpp"
#include "Offsets.hpp"

#include <vector>
#include <shared_mutex>
#include <algorithm>
#include <cmath>
#include <string>
#include <mutex>

class MapParser
{
public:
    explicit MapParser(Memory& memory) : m_memory(memory) {}

    void Scan(uintptr_t workspace);
    bool IsVisible(const Vector3& start, const Vector3& end,
        uintptr_t ignoreModel = 0, bool includePlayers = false,
        const PlayerFrame* players = nullptr) const;
    void Clear();

    const std::vector<ParsedPart>& Parts() const { return m_parts; }

    std::vector<ParsedPart> PartsCopy() const {
        std::shared_lock lock(m_mutex);
        return m_parts;
    }

private:
    void ClassifyShape(const std::string& className, const std::string& nameLower,
        const Vector3& size, uintptr_t instance, ParsedPart& out) const;

    Memory& m_memory;
    mutable std::shared_mutex m_mutex;
    std::vector<ParsedPart> m_parts;
};

