#pragma once

#include <cmath>
#include <cstdint>

struct Vector2
{
    float x = 0.f;
    float y = 0.f;

    Vector2 operator+(const Vector2& o) const { return { x + o.x, y + o.y }; }
    Vector2 operator-(const Vector2& o) const { return { x - o.x, y - o.y }; }
    Vector2 operator*(float s) const { return { x * s, y * s }; }
    float Length() const { return std::sqrt(x * x + y * y); }
};

struct Vector3
{
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;

    Vector3 operator+(const Vector3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vector3 operator-(const Vector3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vector3 operator*(float s) const { return { x * s, y * s, z * s }; }
    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    float LengthSquared() const { return x * x + y * y + z * z; }
};

struct Matrix3
{
    float m[9]{};
};

struct Matrix4
{
    float m[16]{};
};

inline Vector3 Normalize(const Vector3& v)
{
    const float len = v.Length();
    if (len < 1e-6f)
        return {};
    return v * (1.f / len);
}

inline float Dot(const Vector3& a, const Vector3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vector3 Cross(const Vector3& a, const Vector3& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline Matrix3 LookRotation(const Vector3& from, const Vector3& to)
{
    Vector3 forward = Normalize(to - from);
    if (forward.LengthSquared() < 1e-8f)
        forward = { 0.f, 0.f, -1.f };

    Vector3 upRef = { 0.f, 1.f, 0.f };
    if (std::fabs(Dot(forward, upRef)) > 0.99f)
        upRef = { 1.f, 0.f, 0.f };

    const Vector3 right = Normalize(Cross(upRef, forward));
    const Vector3 up = Cross(forward, right);

    Matrix3 r{};
    r.m[0] = -right.x; r.m[1] = up.x; r.m[2] = -forward.x;
    r.m[3] = -right.y; r.m[4] = up.y; r.m[5] = -forward.y;
    r.m[6] = -right.z; r.m[7] = up.z; r.m[8] = -forward.z;
    return r;
}

inline bool AimWorldToScreen(const Vector3& world, Vector2& out, const Matrix4& view, const Vector2& viewport)
{
    const float* m = view.m;
    float w_x = world.x * m[12] + world.y * m[13] + world.z * m[14] + m[15];
    if (w_x < 0.01f)
        return false;

    float screen_x = world.x * m[0] + world.y * m[1] + world.z * m[2] + m[3];
    float screen_y = world.x * m[4] + world.y * m[5] + world.z * m[6] + m[7];
    float inv_w = 1.0f / w_x;
    out.x = (viewport.x * 0.5f * screen_x * inv_w) + (viewport.x * 0.5f);
    out.y = -(viewport.y * 0.5f * screen_y * inv_w) + (viewport.y * 0.5f);
    if (out.x != out.x || out.y != out.y)
        return false;
    return true;
}

inline bool WorldToScreen(const Vector3& world, const Matrix4& view, const Vector2& viewport, Vector2& out)
{
    return AimWorldToScreen(world, out, view, viewport);
}

inline bool WorldToScreen(const Vector3& world, const Matrix4& matrix, float width, float height, Vector2& out)
{
    return AimWorldToScreen(world, out, matrix, Vector2{ width, height });
}

inline float Distance2D(const Vector2& a, const Vector2& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

inline bool RayAabb(const Vector3& origin, const Vector3& dir, float maxT,
    const Vector3& center, const Vector3& half, float& outT)
{
    float tmin = 0.f;
    float tmax = maxT;
    const float o[3] = { origin.x, origin.y, origin.z };
    const float d[3] = { dir.x, dir.y, dir.z };
    const float c[3] = { center.x, center.y, center.z };
    const float h[3] = { half.x, half.y, half.z };

    for (int i = 0; i < 3; ++i)
    {
        const float mn = c[i] - h[i];
        const float mx = c[i] + h[i];
        if (std::fabs(d[i]) < 1e-8f)
        {
            if (o[i] < mn || o[i] > mx)
                return false;
            continue;
        }
        float inv = 1.f / d[i];
        float t1 = (mn - o[i]) * inv;
        float t2 = (mx - o[i]) * inv;
        if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
        if (t1 > tmin) tmin = t1;
        if (t2 < tmax) tmax = t2;
        if (tmin > tmax)
            return false;
    }
    if (tmin < 0.f) tmin = tmax;
    if (tmin <= 0.f || tmin > maxT)
        return false;
    outT = tmin;
    return true;
}

struct Vector2i16
{
    int16_t x = 0;
    int16_t y = 0;
};

