#include "XXRemoveRootMotionModifer.h"
#include "XXRootMotionRemover.h"
#include "XXAnimRootEditor.h"

void UXXRemoveRootMotionModifer::OnApply_Implementation(UAnimSequence* AnimationSequence) {
	if (FrameStart >= FrameEnd) {
		UE_LOG(XXAnimRootEditor, Warning, TEXT("[UXXRemoveRootMotionModifer] Invalid frame!"));
		return;
	}
	Super::OnApply_Implementation(AnimationSequence);
	FXXRootMotionRemover Remover(AnimationSequence);
	EXXRootMotionRemoveFlag Flags = (EXXRootMotionRemoveFlag)((RemoveRotation << 3) | RemoveTranslation);
	Remover.RemoveRootMotion(Flags, FrameStart, FrameEnd);
}
