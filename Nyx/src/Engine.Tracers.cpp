#include "Engine.hpp"
#include "Math.hpp"
#include <algorithm>

void Engine::PushBulletTracer(const Vector3& from, const Vector3& to, float duration)
{
    float life = duration;
    if (life < 0.35f) life = 0.55f;
    if (life > 1.5f) life = 1.5f;

    BulletTracer t{};
    t.from = from;
    t.to = to;
    t.life = life;
    t.maxLife = life;

    TracerFrame frame{};
    {
        std::lock_guard<std::mutex> lock(m_tracerMutex);
        m_tracerLive.push_back(t);
        if (m_tracerLive.size() > 40)
            m_tracerLive.erase(m_tracerLive.begin(),
                m_tracerLive.begin() + static_cast<std::ptrdiff_t>(m_tracerLive.size() - 40));
        frame.items = m_tracerLive;
    }
    frame.sequence = 1;
    // publish immediately so the next render frame can draw it
    m_tracers.Publish(std::move(frame));
}

void Engine::TickTracers(float dt)
{
    if (dt < 0.f) dt = 0.f;
    if (dt > 0.1f) dt = 0.1f; // avoid huge dt wiping all tracers

    TracerFrame frame{};
    {
        std::lock_guard<std::mutex> lock(m_tracerMutex);
        for (size_t i = 0; i < m_tracerLive.size();)
        {
            m_tracerLive[i].life -= dt;
            if (m_tracerLive[i].life <= 0.f)
            {
                m_tracerLive.erase(m_tracerLive.begin() + static_cast<std::ptrdiff_t>(i));
                continue;
            }
            ++i;
        }
        frame.items = m_tracerLive;
    }
    frame.sequence = frame.items.empty() ? 0 : 1;
    m_tracers.Publish(std::move(frame));
}
