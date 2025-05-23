#include "XXRootMotionRemover.h"
#include "XXAnimRootEditor.h"
#include "Animation/Skeleton.h"

static constexpr int32 ROOT_BONE_INDEX = 0;

FXXRootMotionRemover::FXXRootMotionRemover(UAnimSequence* InAnimSequence): AnimSequence(InAnimSequence) {
	check(AnimSequence);
}

FXXRootMotionRemover::~FXXRootMotionRemover() {
}

FTransform FXXRootMotionRemover::GetRootBoneTransform(int32 InFrame) {
	if(USkeleton* Skeleton = AnimSequence->GetSkeleton()) {
		if (IAnimationDataModel* DataModel = AnimSequence->GetDataModel()) {
			FName BoneName = Skeleton->GetReferenceSkeleton().GetBoneName(ROOT_BONE_INDEX);
			FFrameNumber FrameNumber(InFrame);
			return DataModel->GetBoneTrackTransform(BoneName, FrameNumber);
		}
	}
	return FTransform::Identity;
}

bool FXXRootMotionRemover::RemoveRootMotion(EXXRootMotionRemoveFlag Flags, int32 FrameStart, int32 FrameEnd) {
	if (EXXRootMotionRemoveFlag::None == Flags) {
		UE_LOG(XXAnimRootEditor, Warning, TEXT("[RemoveRootMotion] No flags checked!"));
		return false;
	}

	UE_LOG(XXAnimRootEditor, Log, TEXT("[RemoveRootMotion] Flags=%u, Range=[%d, %d)"), (uint8)Flags, FrameStart, FrameEnd);

	check(FrameStart < FrameEnd);
	USkeleton* Skeleton = AnimSequence->GetSkeleton();
	if (!Skeleton) {
		UE_LOG(XXAnimRootEditor, Warning, TEXT("[RemoveRootMotion] Could not find Skeleton!"));
		return false;
	}

	// Lazy cache bone names
	RootBoneName = Skeleton->GetReferenceSkeleton().GetBoneName(ROOT_BONE_INDEX);
	if(ChildBoneNames.IsEmpty()) {
		TArray<int32> ChildBoneIndices;
		Skeleton->GetReferenceSkeleton().GetDirectChildBones(ROOT_BONE_INDEX, ChildBoneIndices);
		ChildBoneNames.Reserve(ChildBoneIndices.Num());
		for(int32 i=0; i<ChildBoneIndices.Num(); ++i) {
			ChildBoneNames.Add(Skeleton->GetReferenceSkeleton().GetBoneName(ChildBoneIndices[i]));
		}
	}

	if (ChildBoneNames.IsEmpty()) {
		UE_LOG(XXAnimRootEditor, Warning, TEXT("[RemoveRootMotion] Could not find child bones!"));
		return false;
	}

	IAnimationDataModel* DataModel = AnimSequence->GetDataModel();
	if (!DataModel) {
		UE_LOG(XXAnimRootEditor, Warning, TEXT("[RemoveRootMotion] Could not find DataModel!"));
		return false;
	}
	FrameStart = FMath::Max(FrameStart, 0);
	FrameEnd = FMath::Min(FrameEnd, DataModel->GetNumberOfKeys());

	TArray<FFrameNumber> FrameNumbers;
	FrameNumbers.SetNumZeroed(FrameEnd - FrameStart);
	for (int32 i = 0; i < FrameNumbers.Num(); ++i) {
		FrameNumbers[i] = FFrameNumber(FrameStart + i);
	}
	// Get the root bone raw data
	TArray<FTransform> RootBoneTransforms;
	DataModel->GetBoneTrackTransforms(RootBoneName, FrameNumbers, RootBoneTransforms);
	// Get children raw data
	TArray<TArray<FTransform>> ChildBonesTransforms;
	ChildBonesTransforms.AddDefaulted(ChildBoneNames.Num());
	for(int32 i=0; i<ChildBonesTransforms.Num(); ++i) {
		DataModel->GetBoneTrackTransforms(ChildBoneNames[i], FrameNumbers, ChildBonesTransforms[i]);
	}

	// Backup transforms
	{
		if (BackupTransforms.IsEmpty()) {
			BackupTransforms.AddDefaulted(DataModel->GetNumberOfKeys());
		}
		for (int32 Frame = FrameStart; Frame < FrameEnd; ++Frame) {
			if (BackupTransforms[Frame].IsEmpty()) {
				TArray<FTransform>& BackupData = BackupTransforms[Frame];
				BackupData.Reserve(1 + ChildBoneNames.Num());
				BackupData.Add({ RootBoneTransforms[Frame - FrameStart] });
				for (int32 i = 0; i < ChildBoneNames.Num(); ++i) {
					BackupData.Add({ ChildBonesTransforms[i][Frame - FrameStart] });
				}
			}
		}
	}

	bool bResult = false;
	for(int32 i=0; i<RootBoneTransforms.Num(); ++i) {
		FTransform& RootTransform = RootBoneTransforms[i];
		if(RootTransform.Equals(FTransform::Identity)) {
			continue;
		}
		FTransform RootTransformOrigin = RootTransform;

		// Translation
		if(EnumHasAnyFlags(Flags, EXXRootMotionRemoveFlag::Translation)) {
			FVector BoneTranslation = RootTransform.GetTranslation();
			if(!BoneTranslation.IsNearlyZero()) {
				if (EnumHasAllFlags(Flags, EXXRootMotionRemoveFlag::TranslationX)) {
					BoneTranslation.X = 0.0;
				}
				if (EnumHasAllFlags(Flags, EXXRootMotionRemoveFlag::TranslationY)) {
					BoneTranslation.Y = 0.0;
				}
				if (EnumHasAllFlags(Flags, EXXRootMotionRemoveFlag::TranslationZ)) {
					BoneTranslation.Z = 0.0;
				}
				RootTransform.SetTranslation(BoneTranslation);
			}
		}

		// Rotaion
		if(EnumHasAnyFlags(Flags, EXXRootMotionRemoveFlag::Rotation)) {
			FRotator BoneRotation = RootTransform.Rotator();
			if(!BoneRotation.IsNearlyZero()) {
				if (EnumHasAllFlags(Flags, EXXRootMotionRemoveFlag::RotationRoll)) {
					BoneRotation.Roll = 0.0;
				}
				if (EnumHasAllFlags(Flags, EXXRootMotionRemoveFlag::RotationPitch)) {
					BoneRotation.Pitch = 0.0;
				}
				if (EnumHasAllFlags(Flags, EXXRootMotionRemoveFlag::RotationYaw)) {
					BoneRotation.Yaw = 0.0;
				}
				RootTransform.SetRotation(BoneRotation.Quaternion());
			}
		}

		if(RootTransformOrigin.Equals(RootTransform)) {
			continue;
		}

		// Accumulate on child bones
		FTransform RootTransformModifiedInverse = RootTransform.Inverse();
		for(TArray<FTransform>& ChildBoneTransforms: ChildBonesTransforms) {
			// Transform to component space with origin transform, then to root space with modified transform.
			FTransform ChildTransformComponentSpace = ChildBoneTransforms[i] * RootTransformOrigin;
			ChildBoneTransforms[i] = ChildTransformComponentSpace * RootTransformModifiedInverse;
		}
		bResult = true;
	}

	if(!bResult) {
		UE_LOG(XXAnimRootEditor, Warning, TEXT("[RemoveRootMotion] Not transform to remove!"));
		return false;
	}

	FInt32Range FrameRange(FrameStart, FrameEnd);
	// Apply to root bone
	bResult &= UpdateBoneTransform(RootBoneName, FrameRange, RootBoneTransforms);
	// Apply to child bones
	for(int i=0; i<ChildBonesTransforms.Num(); ++i) {
		bResult &= UpdateBoneTransform(ChildBoneNames[i], FrameRange, ChildBonesTransforms[i]);
	}
	if(bResult) {
		// Mark as dirty for compressing.
		bResult &= AnimSequence->MarkPackageDirty();
		AnimSequence->PostEditChange();
	}
	return bResult;
}

bool FXXRootMotionRemover::RestoreRootMotion(int32 FrameStart, int32 FrameEnd) {
	bool bResult = false;
	if(BackupTransforms.Num()) {
		FrameStart = FMath::Max(FrameStart, 0);
		FrameEnd = FMath::Min(FrameEnd, BackupTransforms.Num());
		int32 FrameRangeBegin = FrameStart;
		int32 Frame = FrameStart;
		for (; Frame < FrameEnd + 1; ++Frame) {
			// Split if backup is empty at this frame.
			if (Frame >= FrameEnd || BackupTransforms[Frame].IsEmpty()) {
				if (Frame > FrameRangeBegin) {
					TArray<TArray<FTransform>> BoneTransforms; BoneTransforms.AddDefaulted(1 + ChildBoneNames.Num());
					for (int32 TempFrame = FrameRangeBegin; TempFrame < Frame; ++TempFrame) {
						const TArray<FTransform>& TempTransforms = BackupTransforms[TempFrame];
						check(TempTransforms.Num() == BoneTransforms.Num());
						for (int32 i = 0; i < TempTransforms.Num(); ++i) {
							BoneTransforms[i].Add(TempTransforms[i]);
						}
						// Clean memory
						BackupTransforms[TempFrame].Empty();
					}
					for (int32 i = 0; i < BoneTransforms.Num(); ++i) {
						const FName BoneName = 0 == i ? RootBoneName : ChildBoneNames[i - 1];
						UpdateBoneTransform(BoneName, FInt32Range(FrameRangeBegin, Frame), BoneTransforms[i]);
					}
					bResult = true;
				}
				FrameRangeBegin = Frame + 1;
			}
		}
	}
	if (bResult) {
		bResult &= AnimSequence->MarkPackageDirty();
		AnimSequence->PostEditChange();
	}
	else {
		UE_LOG(XXAnimRootEditor, Warning, TEXT("[RestoreRootMotion] No backup transform for restoring!"));
	}
	return bResult;
}

bool FXXRootMotionRemover::UpdateBoneTransform(FName BoneName, FInt32Range FrameRange, const TArray<FTransform>& Transforms) {
	TArray<FVector3f> PosKeys; PosKeys.Reserve(Transforms.Num());
	TArray<FQuat4f> RotKeys; RotKeys.Reserve(Transforms.Num());
	TArray<FVector3f> ScaleKeys; ScaleKeys.Reserve(Transforms.Num());
	for (const FTransform& Transform : Transforms) {
		PosKeys.Add((FVector3f)Transform.GetTranslation());
		RotKeys.Add((FQuat4f)Transform.GetRotation());
		ScaleKeys.Add((FVector3f)Transform.GetScale3D());
	}
	return AnimSequence->GetController().UpdateBoneTrackKeys(BoneName, FrameRange, PosKeys, RotKeys, ScaleKeys, false);
}
