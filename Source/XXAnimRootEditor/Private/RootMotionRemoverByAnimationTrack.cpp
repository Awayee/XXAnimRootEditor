#include "RootMotionRemoverByAnimationTrack.h"
#include "XXAnimRootEditor.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"

FRootMotionRemoverByAnimationTrack::FRootMotionRemoverByAnimationTrack(UAnimSequence* InAnimSequence) : AnimSequence(InAnimSequence) {
}

FRootMotionRemoverByAnimationTrack::~FRootMotionRemoverByAnimationTrack() {
}

FTransform FRootMotionRemoverByAnimationTrack::GetRootBoneTransform(int32 InFrame) {
	FMemMark Mark(FMemStack::Get());

	USkeleton* Skleton = AnimSequence->GetSkeleton();
	int32 NumRequiredBones = Skleton->GetReferenceSkeleton().GetNum();
	TArray<FBoneIndexType> RequiredBoneIndexArray;
	RequiredBoneIndexArray.AddUninitialized(NumRequiredBones);
	for (int32 BoneIndex = 0; BoneIndex < RequiredBoneIndexArray.Num(); ++BoneIndex) {
		RequiredBoneIndexArray[BoneIndex] = static_cast<FBoneIndexType>(BoneIndex);
	}
	FBoneContainer RequiredBones;
	RequiredBones.InitializeTo(RequiredBoneIndexArray, UE::Anim::FCurveFilterSettings(UE::Anim::ECurveFilterMode::None), *Skleton);
	RequiredBones.SetUseRAWData(true);
	RequiredBones.SetUseSourceData(false);
	RequiredBones.SetDisableRetargeting(false);

	FCompactPose CompactPose;
	CompactPose.SetBoneContainer(&RequiredBones);

	FBlendedCurve Curve;
	Curve.InitFrom(RequiredBones);

	UE::Anim::FStackAttributeContainer Attributes;
	FAnimationPoseData PoseData(CompactPose, Curve, Attributes);
	FAnimExtractContext Context(AnimSequence->GetTimeAtFrame(InFrame), false);
	AnimSequence->GetAnimationPose(PoseData, Context);

	return CompactPose[FCompactPoseBoneIndex(0)];
}

bool FRootMotionRemoverByAnimationTrack::RemoveRootMotion(EXXRootMotionRemoveFlag Flags, int32 FrameStart, int32 FrameEnd) {
	check(FrameStart < FrameEnd);
	USkeleton* Skeleton = AnimSequence->GetSkeleton();
	if (!Skeleton) {
		UE_LOG(XXAnimRootEditor, Warning, TEXT("[FRootMotionRemover::RemoveRootMotion] Could not find Skeleton!"));
		return false;
	}

	TArray<int32> ChildBoneIndices;
	Skeleton->GetReferenceSkeleton().GetDirectChildBones(ROOT_BONE_INDEX, ChildBoneIndices);
	if (ChildBoneIndices.IsEmpty()) {
		UE_LOG(XXAnimRootEditor, Warning, TEXT("[FRootMotionRemover::RemoveRootMotion] Could not find child bones!"));
		return false;
	}

	IAnimationDataModel* DataModel = AnimSequence->GetDataModel();
	if (!DataModel) {
		UE_LOG(XXAnimRootEditor, Warning, TEXT("[FRootMotionRemover::RemoveRootMotion] Could not find DataModel!"));
		return false;
	}
	FrameStart = FMath::Max(FrameStart, 0);
	FrameEnd = FMath::Min(FrameEnd, DataModel->GetNumberOfFrames());

	TArray<FBoneAnimationTrack> BoneTracks = DataModel->GetBoneAnimationTracks();
	FBoneAnimationTrack RootBoneTrack;
	if (const FBoneAnimationTrack* FindedTrack = BoneTracks.FindByPredicate([](const FBoneAnimationTrack& Ele) {return Ele.BoneTreeIndex == ROOT_BONE_INDEX; })) {
		RootBoneTrack.Name = FindedTrack->Name;
		RootBoneTrack.InternalTrackData = FindedTrack->InternalTrackData;
	}
	else {
		UE_LOG(XXAnimRootEditor, Warning, TEXT("[FRootMotionRemover::RemoveRootMotion] Failed to find root bone animation track!"));
		return false;
	}
	TArray<FBoneAnimationTrack> ChildBoneTracks; ChildBoneTracks.Reserve(ChildBoneIndices.Num());
	for (int32 i = 0; i < ChildBoneIndices.Num(); ++i) {
		if (const FBoneAnimationTrack* FindedTrack = BoneTracks.FindByPredicate([i](const FBoneAnimationTrack& Ele) {return Ele.BoneTreeIndex == i; })) {
			FBoneAnimationTrack& ChildBoneTrack = ChildBoneTracks.AddDefaulted_GetRef();
			ChildBoneTrack.Name = FindedTrack->Name;
			ChildBoneTrack.InternalTrackData = FindedTrack->InternalTrackData;
		}
	}
	if (0 == ChildBoneTracks.Num()) {
		UE_LOG(XXAnimRootEditor, Warning, TEXT("[FRootMotionRemover::RemoveRootMotion] Failed to find child animation tracks!"));
		return false;
	}

	bool bResult = false;
	for (int32 Frame = FrameStart; Frame < FrameEnd; ++Frame) {
		FVector3f& RootPos = RootBoneTrack.InternalTrackData.PosKeys[Frame];
		FQuat4f& RootRot = RootBoneTrack.InternalTrackData.RotKeys[Frame];
		const FVector3f& RootScale = RootBoneTrack.InternalTrackData.ScaleKeys[Frame];
		const FTransform3f RootTransformOrigin{ RootRot, RootPos, RootScale };
		if(RootTransformOrigin.Equals(FTransform3f::Identity)) {
			continue;
		}

		if (EnumHasAnyFlags(Flags, EXXRootMotionRemoveFlag::Translation)) {
			if (EnumHasAllFlags(Flags, EXXRootMotionRemoveFlag::TranslationX)) {
				RootPos.X = 0.0f;
			}
			if (EnumHasAllFlags(Flags, EXXRootMotionRemoveFlag::TranslationY)) {
				RootPos.Y = 0.0f;
			}
			if (EnumHasAllFlags(Flags, EXXRootMotionRemoveFlag::TranslationZ)) {
				RootPos.Z = 0.0f;
			}
		}

		if (EnumHasAnyFlags(Flags, EXXRootMotionRemoveFlag::Rotation)) {
			FRotator3f Rotator{ RootRot };
			if (EnumHasAllFlags(Flags, EXXRootMotionRemoveFlag::RotationRoll)) {
				Rotator.Roll = 0.0f;
			}
			if (EnumHasAllFlags(Flags, EXXRootMotionRemoveFlag::RotationPitch)) {
				Rotator.Pitch = 0.0f;
			}
			if (EnumHasAllFlags(Flags, EXXRootMotionRemoveFlag::RotationYaw)) {
				Rotator.Yaw = 0.0f;
			}
			RootRot = Rotator.Quaternion();
		}
		const FTransform3f RootTransformInversed = FTransform3f{RootRot, RootPos, RootScale}.Inverse();
		for (FBoneAnimationTrack& ChildBoneTrack : ChildBoneTracks) {
			FVector3f& ChildPos = ChildBoneTrack.InternalTrackData.PosKeys[Frame];
			FQuat4f& ChildRot = ChildBoneTrack.InternalTrackData.RotKeys[Frame];
			const FVector3f ChildScale = ChildBoneTrack.InternalTrackData.ScaleKeys[Frame];
			const FTransform3f ChildTransformComponentSpace = FTransform3f(ChildRot, ChildPos, ChildScale) * RootTransformOrigin;
			const FTransform3f ChildTransform = ChildTransformComponentSpace * RootTransformInversed;
			ChildPos = ChildTransform.GetTranslation();
			ChildRot = ChildTransform.GetRotation();
		}
		bResult = true;
	}

	if(!bResult) {
		return false;
	}
	IAnimationDataController& Controller = AnimSequence->GetController();
	bResult &= Controller.SetBoneTrackKeys(RootBoneTrack.Name,
		RootBoneTrack.InternalTrackData.PosKeys,
		RootBoneTrack.InternalTrackData.RotKeys,
		RootBoneTrack.InternalTrackData.ScaleKeys);
	for (FBoneAnimationTrack& ChildBoneTrack : ChildBoneTracks) {
		bResult &= Controller.SetBoneTrackKeys(ChildBoneTrack.Name,
			ChildBoneTrack.InternalTrackData.PosKeys,
			ChildBoneTrack.InternalTrackData.RotKeys,
			ChildBoneTrack.InternalTrackData.ScaleKeys);
	}

	// Mark as dirty for compressing
	bResult = AnimSequence->MarkPackageDirty();
	AnimSequence->PostEditChange();
	return bResult;
}

bool FRootMotionRemoverByAnimationTrack::RestoreRootMotion(int32 FrameStart, int32 FrameEnd) {
	UE_LOG(XXAnimRootEditor, Warning, TEXT("[FRootMotionRemoverByAnimationTrack::RestoreRootMotion] Not supported!"));
	return false;
}
