#pragma once
#include "CoreMinimal.h"
#include "Animation/Animsequence.h"

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

class FXXRootMotionRemover{
public:
	FXXRootMotionRemover(UAnimSequence* InAnimSequence);
	virtual ~FXXRootMotionRemover();
	virtual FTransform GetRootBoneTransform(int32 InFrame);
	virtual bool RemoveRootMotion(EXXRootMotionRemoveFlag Flags, int32 FrameStart, int32 FrameEnd);
	virtual bool RestoreRootMotion(int32 FrameStart, int32 FrameEnd);
private:
	UAnimSequence* AnimSequence;
	FName RootBoneName; // "root"
	TArray<FName> ChildBoneNames; // ["hip"]
	TArray<TArray<FTransform>> BackupTransforms; // Backup for restoring, <frame index, [<BoneName, Transform>]
	bool UpdateBoneTransform(FName BoneName, FInt32Range FrameRange, const TArray<FTransform>& Transforms);
};
