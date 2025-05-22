#pragma once
#include "CoreMinimal.h"
#include "Misc/EnumClassFlags.h"

// Remove the transform of root bone in an animation sequence, then keep the pose by accumulating removed transform on the first child of root (hip).

enum class EXXRootMotionRemoveFlag : uint8 {
	None = 0,
	TranslationX = 1,
	TranslationY = 2,
	TranslationZ = 4,
	RotationRoll = 8,
	RotationPitch = 16,
	RotationYaw = 32,

	Translation = TranslationX | TranslationY | TranslationZ,
	Rotation = RotationRoll | RotationPitch | RotationYaw,
	All = 0xff
};
ENUM_CLASS_FLAGS(EXXRootMotionRemoveFlag);

static constexpr int32 ROOT_BONE_INDEX = 0;

class IRootMotionRemover{
public:
	virtual ~IRootMotionRemover() = default;
	virtual FTransform GetRootBoneTransform(int32 InFrame) = 0;
	// The range is [FrameStart, FrameEnd)
	virtual bool RemoveRootMotion(EXXRootMotionRemoveFlag Flags, int32 FrameStart, int32 FrameEnd) = 0;
	virtual bool RestoreRootMotion(int32 FrameStart, int32 FrameEnd) = 0;
};
