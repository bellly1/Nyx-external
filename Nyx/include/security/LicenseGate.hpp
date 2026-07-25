#pragma once

// =============================================================================
// Layer: License gate (separates licensing from feature/UI code)
// =============================================================================
// Feature code (aim, overlay, etc.) only asks: AllowSensitiveOps()?
// It must not parse license JSON or store secrets.
//
// Hot path is O(1): atomics only. Heavy integrity runs on background tick.
//
// Server-side verification remains authoritative (EasyAuth HTTPS).
// This gate only enforces "we have a live seal + session capability".
// =============================================================================

#include "SessionToken.hpp"
#include "IntegrityMonitor.hpp"
#include "AntiTamper.hpp"
#include "ProtectorMarkers.hpp"
#include <atomic>

namespace Security
{

class LicenseGate
{
public:
    static LicenseGate& instance()
    {
        static LicenseGate g;
        return g;
    }

    LicenseGate(const LicenseGate&) = delete;
    LicenseGate& operator=(const LicenseGate&) = delete;

    // After successful server auth
    SEC_NOINLINE void onLicensed(std::string serverToken, std::string hwid)
    {
        SEC_VMP_BEGIN("LicenseGate_onLicensed");
        m_session.setFromServer(std::move(serverToken));
        m_session.setHwid(std::move(hwid));
        m_integrity.Lock();
        m_anti.reset();
        m_active.store(true, std::memory_order_release);
        SEC_VMP_END();
    }

    void revoke()
    {
        m_active.store(false, std::memory_order_release);
        m_session.clear();
        m_anti.markCompromised();
    }

    // HOT PATH — call from aim/features every tick (must stay cheap)
    [[nodiscard]] bool allowSensitiveOps() const noexcept
    {
        if (!m_active.load(std::memory_order_acquire))
            return false;
        if (!m_session.isBound())
            return false;
        if (m_anti.isCompromised())
            return false;
        if (AntiTamper::DebuggerPresentLight())
            return false;
        return true;
    }

    // Background (every few seconds) — kill RE tools + integrity
    void backgroundTick()
    {
        // Always scan for crackers (even before license) so tools get closed
        if (m_anti.enforceProtection())
        {
            // Tools were killed and/or debugger present — sensitive ops blocked
            return;
        }

        if (!m_active.load(std::memory_order_acquire))
            return;

        if (m_integrity.IsLocked() && !m_integrity.Verify())
        {
            m_anti.markCompromised();
            return;
        }
    }

    [[nodiscard]] SessionToken& session() noexcept { return m_session; }
    [[nodiscard]] const SessionToken& session() const noexcept { return m_session; }
    [[nodiscard]] IntegrityMonitor& integrity() noexcept { return m_integrity; }
    [[nodiscard]] bool active() const noexcept { return m_active.load(std::memory_order_acquire); }

private:
    LicenseGate() = default;

    SessionToken m_session;
    IntegrityMonitor m_integrity;
    AntiTamper m_anti;
    std::atomic<bool> m_active{ false };
};

} // namespace Security
