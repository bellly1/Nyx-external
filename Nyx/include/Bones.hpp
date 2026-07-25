#pragma once

#include "Math.hpp"
#include <cstddef>

enum class BoneId : int
{
    Head = 0,
    UpperTorso,
    LowerTorso,
    LeftUpperArm,
    LeftLowerArm,
    LeftHand,
    RightUpperArm,
    RightLowerArm,
    RightHand,
    LeftUpperLeg,
    LeftLowerLeg,
    LeftFoot,
    RightUpperLeg,
    RightLowerLeg,
    RightFoot,
    Count
};

inline constexpr int kBoneCount = static_cast<int>(BoneId::Count);

struct BoneJoint
{
    Vector3 position{};
    // Physical part data retained for size-aware ESP bounds. Position remains
    // the adjusted joint point used by skeleton and aim rendering.
    Vector3 boundsPosition{};
    Vector3 size{};
    bool valid = false;
};

struct BoneEdge
{
    BoneId from;
    BoneId to;
};

inline constexpr BoneEdge kSkeletonEdges[] = {
    { BoneId::Head, BoneId::UpperTorso },
    { BoneId::UpperTorso, BoneId::LowerTorso },
    { BoneId::UpperTorso, BoneId::LeftUpperArm },
    { BoneId::LeftUpperArm, BoneId::LeftLowerArm },
    { BoneId::LeftLowerArm, BoneId::LeftHand },
    { BoneId::UpperTorso, BoneId::RightUpperArm },
    { BoneId::RightUpperArm, BoneId::RightLowerArm },
    { BoneId::RightLowerArm, BoneId::RightHand },
    { BoneId::LowerTorso, BoneId::LeftUpperLeg },
    { BoneId::LeftUpperLeg, BoneId::LeftLowerLeg },
    { BoneId::LeftLowerLeg, BoneId::LeftFoot },
    { BoneId::LowerTorso, BoneId::RightUpperLeg },
    { BoneId::RightUpperLeg, BoneId::RightLowerLeg },
    { BoneId::RightLowerLeg, BoneId::RightFoot },
};

inline constexpr size_t kSkeletonEdgeCount = sizeof(kSkeletonEdges) / sizeof(kSkeletonEdges[0]);

