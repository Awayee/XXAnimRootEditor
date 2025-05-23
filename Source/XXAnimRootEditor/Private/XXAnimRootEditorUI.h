#pragma once
#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Widgets/SCompoundWidget.h"
#include "SEditorViewport.h"
#include "EditorViewportClient.h"
#include "PreviewScene.h"

class SCheckBox;
class UAnimSequence;
class FContentBrowserModule;
class FAssetRegistryModule;
class USkeletalMesh;
class UDebugSkelMeshComponent;
class UArrowComponent;
class UStaticMeshComponent;
class UAnimPreviewInstance;
class SSlider;
class SMultiFlagsCheckBox;
class FXXRootMotionRemover;

DECLARE_DELEGATE_OneParam(FViewportAnimTick, float)

// Viewport
class FXXAnimRootEditorViewportClient : public FEditorViewportClient {
public:
	FXXAnimRootEditorViewportClient(FPreviewScene* PreviewScene);
	void SetAsset(UAnimSequence* AnimAsset);
	void ResetPlay();
	void ResetView();
	virtual void Tick(float DeltaSeconds) override;
	FViewportAnimTick& GetOnAnimTick();
	void CameraFocus();
	void SetPlaying(bool bPlaying);
	void SetPlayTime(float InTime);
	void SetCameraFollow(bool bFllow);
private:
	UDebugSkelMeshComponent* SklMeshComp;
	UStaticMeshComponent* EditorFloorComp;
	UArrowComponent* ArrowComp;
	FVector ArrowLastLocation;
	float CameraDistance;// Distance to box center, for calculating location relative to mesh
	FViewportAnimTick OnAnimTick;
	bool CameraFollow;
	UAnimPreviewInstance* GetAnimPreviewInstance();
	void UpdateSklMeshTransform(); // Update transform by rootmotion
};

// Details panel contains buttons and descriptions.
class SXXAnimRootEditorDetails : public SCompoundWidget {
public:
	SLATE_BEGIN_ARGS(SXXAnimRootEditorDetails) {}
	SLATE_END_ARGS();
	void Construct(const FArguments& InArgs);
	void SetAsset(UAnimSequence* AnimAsset);
	void SetupViewport(FXXAnimRootEditorViewportClient* InPlayInteface);
	void SetPosition(float Position);
private:
	UAnimSequence* CurrentAnimAsset;
	TUniquePtr<FXXRootMotionRemover> RootMotionRemover;
	// The range for display is [FrameStart, FrameEnd]
	int32 FrameStart;
	int32 FrameEnd;
	int32 FrameCount;
	int32 FrameNow;
	FVector RootBoneLocation;
	FVector RootBoneRotation;
	TSharedPtr<SCheckBox> PlayingCheckBox;
	TSharedPtr<SSlider> PlayingSlider;
	TSharedPtr<SMultiFlagsCheckBox> TranslationConfigCheckBox;
	TSharedPtr<SMultiFlagsCheckBox> RotationConfigCheckBox;
	FXXAnimRootEditorViewportClient* Viewport;

	void ManualPause();
	void ManualReset(); // Call aflter modifying
	void SetCurrentFrame(int InFrame);

	// Trigger from widgets
	FReply OnRemoveRootMotion();
	FReply OnRestoreRootMotion();
	void WidgetCameraFollowChanged(ECheckBoxState InState);
	void WidgetSetCurrentFrame(float Value);
	void WidgetPlayStateChanged(ECheckBoxState InState);
	FReply WidgetPlayBack();
	FReply WidgetPlayForward();
	FReply WidgetSetBeginFrame();
	FReply WidgetSetEndFrame();
};

// Parent widget, responsible for listening on asset events. 
class SXXAnimRootEditorUI : public SCompoundWidget {
public:
	SLATE_BEGIN_ARGS(SXXAnimRootEditorUI) {}
		SLATE_ATTRIBUTE(FContentBrowserModule*, ContentBrowser);
		SLATE_ATTRIBUTE(FAssetRegistryModule*, AssetRegistry);
	SLATE_END_ARGS();
	SXXAnimRootEditorUI();
	~SXXAnimRootEditorUI();
	void Construct(const FArguments& InArgs);
	void SetAsset(UAnimSequence* Asset);
private:
	UAnimSequence* EditAnim;
	UAnimSequence* CurrentAnimAsset;
	FContentBrowserModule* ContentBroserModule;
	FAssetRegistryModule* AssetRegistryModule;
	TSharedPtr<SXXAnimRootEditorDetails> WidgetDetails;
	void UpdateSelectedAnimAsset(const TArray<FAssetData>& SelectedAssets);
	void OnSelectedAssetsChanged(const TArray<FAssetData>& SelectedAssets, bool bIsPrimary);
	void OnAssetRemoved(const FAssetData& AssetData);
};