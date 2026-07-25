#pragma once

// =============================================================================
// Layer: Security runtime orchestrator
// =============================================================================
// Single entry for app bootstrap / shutdown of protection subsystems.
// Does not own networking (EasyAuth) or UI — only security services.
// =============================================================================

#include "LicenseGate.hpp"
#include "HwId.hpp"
#include <atomic>
#include <thread>
#include <chrono>

namespace Security
{

class SecurityRuntime
{
public:
    static SecurityRuntime& instance()
    {
        static SecurityRuntime r;
        return r;
    }

    SecurityRuntime(const SecurityRuntime&) = delete;
    SecurityRuntime& operator=(const SecurityRuntime&) = delete;

    // Call early (after console init). Safe in Release; lightweight.
    bool start()
    {
        if (m_running.exchange(true))
            return true;

        m_integrity.CaptureSelf();
        m_thread = std::thread([this] { worker(); });
        return true;
    }

    void stop()
    {
        m_running.store(false, std::memory_order_release);
        if (m_thread.joinable())
            m_thread.join();
        LicenseGate::instance().revoke();
    }

    // Wire after server accepts key
    void notifyLicensed(std::string token, std::string hwid)
    {
        LicenseGate::instance().onLicensed(std::move(token), std::move(hwid));
    }

    [[nodiscard]] static std::string hardwareId()
    {
        return HwId::Collect();
    }

    IntegrityMonitor& earlyIntegrity() noexcept { return m_integrity; }

private:
    SecurityRuntime() = default;

    void worker()
    {
        // Short delay so console can print; then continuous protect
        for (int i = 0; i < 2 && m_running.load(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

        while (m_running.load(std::memory_order_acquire))
        {
            // ~2s interval — kill RE tools + integrity
            for (int i = 0; i < 4 && m_running.load(); ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

            if (!m_running.load())
                break;

            // Always neutralize blacklisted tools (even before login)
            AntiTamper::NeutralizeCrackerProcesses();
            LicenseGate::instance().backgroundTick();
        }
    }

    IntegrityMonitor m_integrity;
    std::atomic<bool> m_running{ false };
    std::thread m_thread;
};

// Dev-only logging (compiled out in Release / NDEBUG)
#if defined(NDEBUG)
#  define SEC_LOG(...) ((void)0)
#else
#  include <cstdio>
#  define SEC_LOG(...) do { std::printf("[sec] "); std::printf(__VA_ARGS__); std::printf("\n"); } while (0)
#endif

} // namespace Security
