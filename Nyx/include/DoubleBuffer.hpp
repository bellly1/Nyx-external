#pragma once

#include <memory>
#include <mutex>
#include <utility>

template <typename T>
class DoubleBuffer
{
public:
    DoubleBuffer()
        : m_read(std::make_shared<const T>())
    {
    }

    void Publish(T value)
    {
        auto next = std::make_shared<const T>(std::move(value));
        std::lock_guard<std::mutex> lock(m_mutex);
        m_read = std::move(next);
    }

    std::shared_ptr<const T> Read() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_read;
    }

private:
    mutable std::mutex m_mutex;
    std::shared_ptr<const T> m_read;
};

