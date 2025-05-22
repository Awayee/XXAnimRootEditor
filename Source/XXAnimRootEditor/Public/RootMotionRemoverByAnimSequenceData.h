#pragma once
#include "IRootMotionRemover.h"
#include "Animation/Animsequence.h"
class FRootMotionRemoverByAnimSequenceData : public IRootMotionRemover{
public:
	FRootMotionRemoverByAnimSequenceData(UAnimSequence* InAnimSequence);
	virtual ~FRootMotionRemoverByAnimSequenceData() override;
	virtual FTransform GetRootBoneTransform(int32 InFrame) override;
	virtual bool RemoveRootMotion(EXXRootMotionRemoveFlag Flags, int32 FrameStart, int32 FrameEnd) override;
	virtual bool RestoreRootMotion(int32 FrameStart, int32 FrameEnd) override;
private:
	UAnimSequence* AnimSequence;
	FName RootBoneName; // "root"
	TArray<FName> ChildBoneNames; // ["hip"]
	TArray<TArray<FTransform>> BackupTransforms; // Backup for restoring, <frame index, [<BoneName, Transform>]
	bool UpdateBoneTransform(FName BoneName, FInt32Range FrameRange, const TArray<FTransform>& Transforms);
};
