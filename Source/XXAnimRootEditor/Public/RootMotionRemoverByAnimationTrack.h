#pragma once
#include "IRootMotionRemover.h"
#include "Animation/Animsequence.h"
class FRootMotionRemoverByAnimationTrack : public IRootMotionRemover{
public:
	FRootMotionRemoverByAnimationTrack(UAnimSequence* InAnimSequence);
	virtual ~FRootMotionRemoverByAnimationTrack() override;
	virtual FTransform GetRootBoneTransform(int32 InFrame) override;
	virtual bool RemoveRootMotion(EXXRootMotionRemoveFlag Flags, int32 FrameStart, int32 FrameEnd) override;
	virtual bool RestoreRootMotion(int32 FrameStart, int32 FrameEnd) override;
private:
	UAnimSequence* AnimSequence;
};
