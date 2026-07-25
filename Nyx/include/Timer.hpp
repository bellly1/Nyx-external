#pragma once

#include <chrono>
#include <atomic>
#include <cstdint>

class Timer
{
public:
    void Start()
    {
        m_start = std::chrono::steady_clock::now();
        m_running = true;
    }

    double End()
    {
        if (!m_running)
            return m_lastMs;

        const auto end = std::chrono::steady_clock::now();
        m_lastMs = std::chrono::duration<double, std::milli>(end - m_start).count();
        m_running = false;
        return m_lastMs;
    }

    double ElapsedMs() const
    {
        if (!m_running)
            return m_lastMs;
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - m_start).count();
    }

    double LastMs() const { return m_lastMs; }

private:
    std::chrono::steady_clock::time_point m_start{};
    double m_lastMs = 0.0;
    bool m_running = false;
};

struct ThreadStats
{
    std::atomic<double> lastMs{ 0.0 };
    std::atomic<double> avgMs{ 0.0 };
    std::atomic<double> maxMs{ 0.0 };
    std::atomic<uint64_t> ticks{ 0 };

    void Record(double ms)
    {
        lastMs.store(ms, std::memory_order_relaxed);
        ticks.fetch_add(1, std::memory_order_relaxed);

        const double prev = avgMs.load(std::memory_order_relaxed);
        avgMs.store(prev * 0.9 + ms * 0.1, std::memory_order_relaxed);

        double curMax = maxMs.load(std::memory_order_relaxed);
        while (ms > curMax && !maxMs.compare_exchange_weak(curMax, ms, std::memory_order_relaxed))
        {
        }
    }
};

