#include "XXAnimRootEditorUI.h"
#include "XXAnimRootEditorStyle.h"
#include "XXAnimRootEditor.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Input/SSlider.h"
#include "Animation/AnimSequence.h"
#include "Components/ArrowComponent.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AnimPreviewInstance.h"
#include "RootMotionRemoverByAnimSequenceData.h"
#include "SMultiFlagsCheckBox.h"

// UnrealEd
#include "EditorViewportClient.h"
#include "AssetEditorModeManager.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "EditorViewportCommands.h"

#define LOCTEXT_NAMESPACE "FXXAnimRootEditorModule"
static const FVector3d DefaultViewEuler{ 0.0, -20.0, -90.0 };

// Viewport
class SXXAnimRootEditorViewport : public SEditorViewport {
public:
	SLATE_BEGIN_ARGS(SXXAnimRootEditorViewport) {}
	SLATE_END_ARGS();
	void Construct(const FArguments& InArgs) {
		PreviewScene = MakeShared<FPreviewScene>();
		SEditorViewport::Construct(SEditorViewport::FArguments());
	}
	FXXAnimRootEditorViewportClient* GetViewportClientOverride() {
		return (FXXAnimRootEditorViewportClient*)Client.Get();
	}
private:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override {
		return MakeShareable(new FXXAnimRootEditorViewportClient(PreviewScene.Get()));
	}
	virtual void BindCommands() override {
		SEditorViewport::BindCommands();
		CommandList->MapAction(FEditorViewportCommands::Get().FocusViewportToSelection,
			FExecuteAction::CreateRaw(GetViewportClientOverride(), &FXXAnimRootEditorViewportClient::CameraFocus));
	}
	TSharedPtr<FPreviewScene> PreviewScene;
};


// Viewport
FXXAnimRootEditorViewportClient::FXXAnimRootEditorViewportClient(FPreviewScene* InPreviewScene): FEditorViewportClient(nullptr, InPreviewScene, nullptr){
	((FAssetEditorModeManager*)ModeTools.Get())->SetPreviewScene(InPreviewScene);
	EngineShowFlags.DisableAdvancedFeatures();
	PreviewScene->SetSkyCubemap(GUnrealEd->GetThumbnailManager()->AmbientCubemap);
	DrawHelper.bDrawGrid = true;// Draw grid of ground
	DrawHelper.bDrawPivot = true;// Draw axis
	DrawHelper.PerspectiveGridSize = UE_OLD_HALF_WORLD_MAX1;
	SetRealtime(true); // Realtime mode

	// AddComponents
	// skl
	SklMeshComp = NewObject<UDebugSkelMeshComponent>(GetWorld(), TEXT("PreviewMesh"));
	PreviewScene->AddComponent(SklMeshComp, FTransform::Identity, false);
	SklMeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	SklMeshComp->SetProcessRootMotionMode(EProcessRootMotionMode::LoopAndReset);
	// ground
	EditorFloorComp = NewObject<UStaticMeshComponent>(GetWorld(), TEXT("EditorFloorComp"));
	UStaticMesh* FloorMesh = LoadObject<UStaticMesh>(NULL, TEXT("/Engine/EditorMeshes/PhAT_FloorBox.PhAT_FloorBox"), NULL, LOAD_None, NULL);
	if (ensure(FloorMesh)){
		EditorFloorComp->SetStaticMesh(FloorMesh);
	}
	UMaterial* Material = LoadObject<UMaterial>(NULL, TEXT("/Engine/EditorMaterials/PersonaFloorMat.PersonaFloorMat"), NULL, LOAD_None, NULL);
	if (ensure(Material)){
		EditorFloorComp->SetMaterial(0, Material);
	}
	EditorFloorComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewScene->AddComponent(EditorFloorComp, FTransform::Identity, false);
	// arrow
	ArrowComp = NewObject<UArrowComponent>(GetWorld(), TEXT("Arrow"));
	ArrowComp->SetArrowColor(FLinearColor::Green);
	// The arrow orientates at X axis initially, so rotate it
	FTransform ArrowTransform = FTransform::Identity;
	ArrowTransform.SetRotation(FQuat::MakeFromRotationVector(FVector3d(0, 0, PI/2)));
	PreviewScene->AddComponent(ArrowComp, ArrowTransform, false);

	GetWorld()->SetBegunPlay(false);
}

void FXXAnimRootEditorViewportClient::SetAsset(UAnimSequence* AnimAsset) {
	USkeletalMesh* SklMeshAsset = AnimAsset->GetPreviewMesh();
	if (!SklMeshAsset) {
		SklMeshAsset = AnimAsset->GetSkeleton()->GetPreviewMesh();
	}
	SklMeshComp->SetSkeletalMesh(SklMeshAsset);
	SklMeshComp->InitAnim(true);
	// Only PreviewInstance works
	SklMeshComp->EnablePreview(true, AnimAsset);
	ArrowComp->AttachToComponent(SklMeshComp, FAttachmentTransformRules::KeepRelativeTransform, FName("root"));
	ResetView();
	// Update scene
	Invalidate();
}

void FXXAnimRootEditorViewportClient::ResetView() {
	SklMeshComp->SetWorldTransform(FTransform::Identity);
	SetLookAtLocation(FVector::Zero());
	const FVector Extent = SklMeshComp->GetLocalBounds().GetBox().GetExtent();
	// Scale the height view to contain the mesh.
	const float BoxHeight = Extent.Z * 2.5f;
	const float TanHalfFov = FMath::Tan(FMath::DegreesToRadians(ViewFOV * 0.5f));
	const float DistanceToCenter = BoxHeight / TanHalfFov;
	const FRotator ViewRotation = FRotator::MakeFromEuler(DefaultViewEuler);
	SetViewRotation(ViewRotation);
	const FVector ViewForward = ViewRotation.Quaternion().GetForwardVector();
	const FVector ViewLocation = -ViewForward * DistanceToCenter;
	SetViewLocation(ViewLocation);
	CameraDistance = DistanceToCenter;
	ArrowLastLocation = ArrowComp->GetComponentLocation();
}

void FXXAnimRootEditorViewportClient::Tick(float DeltaSeconds) {
	FEditorViewportClient::Tick(DeltaSeconds);
	PreviewScene->UpdateCaptureContents();
	PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	if (UAnimPreviewInstance* Inst = GetAnimPreviewInstance(); Inst->IsPlaying() && Inst->GetPlayRate() > 0.0f) {
		UpdateSklMeshTransform();
		// Trigger animation tick event
		OnAnimTick.ExecuteIfBound(Inst->GetCurrentTime());
	}
}

FViewportAnimTick& FXXAnimRootEditorViewportClient::GetOnAnimTick() {
	return OnAnimTick;
}

void FXXAnimRootEditorViewportClient::CameraFocus() {
	const FVector ComponentLocation = ArrowComp->GetComponentLocation();
	SetLookAtLocation(ComponentLocation);
	FVector CameraForward = GetViewRotation().Quaternion().GetForwardVector();
	SetViewLocation(ComponentLocation - CameraForward * CameraDistance);
}

void FXXAnimRootEditorViewportClient::SetPlaying(bool bPlaying) {
	if (UAnimPreviewInstance* Inst = GetAnimPreviewInstance()) {
		Inst->SetPlaying(bPlaying);
	}
}

void FXXAnimRootEditorViewportClient::SetPlayTime(float InTime) {
	if (UAnimPreviewInstance* Inst = GetAnimPreviewInstance()) {
		Inst->SetPosition(InTime, false);
		Inst->SetPlaying(false);// Stop playing to use anim extracted rootmotion
		UpdateSklMeshTransform();
		Invalidate();
	}
}

UAnimPreviewInstance* FXXAnimRootEditorViewportClient::GetAnimPreviewInstance() {
	return SklMeshComp ? SklMeshComp->PreviewInstance : nullptr;
}

void FXXAnimRootEditorViewportClient::UpdateSklMeshTransform() {
	// Handle updating the preview component to represent the effects of root motion
	const FBoxSphereBounds Bounds = EditorFloorComp->CalcBounds(EditorFloorComp->GetComponentTransform());
	SklMeshComp->ConsumeRootMotion(Bounds.GetBox().Min, Bounds.GetBox().Max);
	// Focus on arrow location
	const FVector ArrowLocation = ArrowComp->GetComponentLocation();
	SetViewLocation(GetViewLocation() + ArrowLocation - ArrowLastLocation);
	SetLookAtLocation(ArrowLocation);
	ArrowLastLocation = ArrowLocation;
}

// Details
void SXXAnimRootEditorDetails::Construct(const FArguments& InArgs) {
	const FSlateBrush* BrushPlay = FXXAnimRootEditorStyle::Get().GetBrush("XXAnimRootEditor.Play");
	const FSlateBrush* BrushPause = FXXAnimRootEditorStyle::Get().GetBrush("XXAnimRootEditor.Pause");
	ChildSlot[
		SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[
				SNew(STextBlock).Text_Lambda([this]() {
					return FText::Format(LOCTEXT("AssetName", "AssetName: {0}"), FText::FromString(CurrentAnimAsset->GetName())); })
			]
			+ SVerticalBox::Slot().AutoHeight()[
				SNew(SBorder)
			]
			+ SVerticalBox::Slot().AutoHeight()[
				SNew(STextBlock).Text_Lambda([this]() {
					return FText::Format(LOCTEXT("Location", "Root Location ({0}, {1}, {2})"), RootBoneLocation.X, RootBoneLocation.Y, RootBoneLocation.Z); })
			]
			+ SVerticalBox::Slot().AutoHeight()[
				SNew(STextBlock).Text_Lambda([this]() {
					return FText::Format(LOCTEXT("Location", "Root Rotation ({0}, {1}, {2})"), RootBoneRotation.X, RootBoneRotation.Y, RootBoneRotation.Z); })
			]
			+ SVerticalBox::Slot().AutoHeight()[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()[
						SAssignNew(PlayingSlider, SSlider)
							.OnControllerCaptureBegin_Raw(this, &SXXAnimRootEditorDetails::ManualPause)
							.OnMouseCaptureBegin_Raw(this, &SXXAnimRootEditorDetails::ManualPause)
							.OnValueChanged_Raw(this, &SXXAnimRootEditorDetails::WidgetSetCurrentFrame)
					]
					+ SHorizontalBox::Slot().AutoWidth()[
						SNew(STextBlock).Text_Lambda([this]() {return FText::Format(LOCTEXT("Frame", "Frame {0}"), FrameNow); })
					]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[
						SNew(SButton)
							.Text_Lambda([this]() { return FText::Format(LOCTEXT("FrameStart", "From {0}"), FrameStart); })
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							.OnClicked_Raw(this, &SXXAnimRootEditorDetails::WidgetSetBeginFrame)
					]
					+ SHorizontalBox::Slot().AutoWidth()[
						SNew(SButton)
							.OnClicked_Raw(this, &SXXAnimRootEditorDetails::WidgetPlayBack)
							.Content()[
								SNew(SImage).Image(FXXAnimRootEditorStyle::Get().GetBrush("XXAnimRootEditor.Left"))
							]
					]
					+ SHorizontalBox::Slot().AutoWidth()[
						SAssignNew(PlayingCheckBox, SCheckBox)
							.UncheckedImage(BrushPlay)
							.UncheckedHoveredImage(BrushPlay)
							.UncheckedPressedImage(BrushPlay)
							.CheckedImage(BrushPause)
							.CheckedHoveredImage(BrushPause)
							.CheckedPressedImage(BrushPause)
							.OnCheckStateChanged_Raw(this, &SXXAnimRootEditorDetails::WidgetPlayStateChanged)
							.IsChecked(false)
					]
					+ SHorizontalBox::Slot().AutoWidth()[
						SNew(SButton)
							.OnClicked_Raw(this, &SXXAnimRootEditorDetails::WidgetPlayForward)
							.Content()[
								SNew(SImage).Image(FXXAnimRootEditorStyle::Get().GetBrush("XXAnimRootEditor.Right"))
							]
					]
					+ SHorizontalBox::Slot().AutoWidth()[
						SNew(SButton)
							.Text_Lambda([this]() { return FText::Format(LOCTEXT("FrameEnd", "To {0}"), FrameEnd); })
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							.OnClicked_Raw(this, &SXXAnimRootEditorDetails::WidgetSetEndFrame)
					]
			]
			+ SVerticalBox::Slot().AutoHeight()[
				SNew(SBorder)
			]
			+ SVerticalBox::Slot().AutoHeight()[
				SNew(STextBlock).Text(LOCTEXT("RootMotionConfig", "Remove Root Motion Config:"))
			]
			+ SVerticalBox::Slot().AutoHeight()[
				SAssignNew(TranslationConfigCheckBox, SMultiFlagsCheckBox)
					.NumItem(3).IsChecked(false)
					.Label(LOCTEXT("Translation", "Translation"))
			]
			+ SVerticalBox::Slot().AutoHeight()[
				SAssignNew(RotationConfigCheckBox, SMultiFlagsCheckBox)
					.NumItem(3).IsChecked(false)
					.Label(LOCTEXT("Rotation", "Rotation"))
			]
			+ SVerticalBox::Slot().AutoHeight()[
				SNew(SButton)
					.Text(LOCTEXT("RemoveRootMotion", "Remove Root Motion"))
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					.OnClicked_Raw(this, &SXXAnimRootEditorDetails::OnRemoveRootMotion)
			]
			+ SVerticalBox::Slot().AutoHeight()[
				SNew(SButton)
					.Text(LOCTEXT("RestoreRootMotion", "Restore Root Motion"))
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					.OnClicked_Raw(this, &SXXAnimRootEditorDetails::OnRestoreRootMotion)
			]
	];
	TranslationConfigCheckBox->SetItemText(0, FText::FromString(TEXT("X")));
	TranslationConfigCheckBox->SetItemText(1, FText::FromString(TEXT("Y")));
	TranslationConfigCheckBox->SetItemText(2, FText::FromString(TEXT("Z")));
	RotationConfigCheckBox->SetItemText(0, LOCTEXT("Roll", "Roll"));
	RotationConfigCheckBox->SetItemText(1, LOCTEXT("Pitch", "Pitch"));
	RotationConfigCheckBox->SetItemText(2, LOCTEXT("Yaw", "Yaw"));
}

void SXXAnimRootEditorDetails::SetAsset(UAnimSequence* AnimAsset) {
	CurrentAnimAsset = AnimAsset;
	RootMotionRemover.Reset(new FRootMotionRemoverByAnimSequenceData(AnimAsset));
	FrameCount = AnimAsset->GetNumberOfSampledKeys();
	FrameStart = 0;
	FrameEnd = FrameCount - 1;
	PlayingSlider->SetMinAndMaxValues(0.0f, (float)FrameCount);
	SetCurrentFrame(0);
	if (Viewport) {
		Viewport->SetAsset(AnimAsset);
		Viewport->SetPlaying(PlayingCheckBox->IsChecked());
	}
}

void SXXAnimRootEditorDetails::SetupViewport(FXXAnimRootEditorViewportClient* InViewport) {
	Viewport = InViewport;
	Viewport->GetOnAnimTick().BindRaw(this, &SXXAnimRootEditorDetails::SetPosition);
}

void SXXAnimRootEditorDetails::SetPosition(float Position) {
	int Frame = CurrentAnimAsset->GetFrameAtTime(Position);
	SetCurrentFrame(Frame);
	PlayingSlider->SetValue((float)FrameNow);
}

void SXXAnimRootEditorDetails::ManualPause() {
	PlayingCheckBox->SetIsChecked(ECheckBoxState::Unchecked);
	if (Viewport) {
		Viewport->SetPlaying(false);
	}
}

void SXXAnimRootEditorDetails::ManualReset() {
	SetCurrentFrame(0);
	if (Viewport) {
		Viewport->SetPlayTime(0.0f);
		Viewport->ResetView();
	}
	ManualPause();
}

void SXXAnimRootEditorDetails::SetCurrentFrame(int InFrame) {
	FrameNow = FMath::Clamp(InFrame, 0, FrameCount-1);
	FTransform RootBoneTransform = RootMotionRemover->GetRootBoneTransform(FrameNow);
	RootBoneLocation = RootBoneTransform.GetLocation();
	RootBoneRotation = RootBoneTransform.GetRotation().Euler();
	PlayingSlider->SetValue((float)FrameNow);
}

FReply SXXAnimRootEditorDetails::OnRemoveRootMotion() {
	const uint32 TranslationFlags = TranslationConfigCheckBox->GetFlags();
	const uint32 RotationFlags = RotationConfigCheckBox->GetFlags();
	const EXXRootMotionRemoveFlag Flags = (EXXRootMotionRemoveFlag)(RotationFlags << 3 | TranslationFlags);
	RootMotionRemover->RemoveRootMotion(Flags, FrameStart, FrameEnd+1);
	ManualReset();
	return FReply::Handled();
}

FReply SXXAnimRootEditorDetails::OnRestoreRootMotion() {
	RootMotionRemover->RestoreRootMotion(FrameStart, FrameEnd+1);
	ManualReset();
	return FReply::Handled();
}

void SXXAnimRootEditorDetails::WidgetSetCurrentFrame(float Value) {
	SetCurrentFrame(Value);
	if (Viewport) {
		Viewport->SetPlayTime(CurrentAnimAsset->GetTimeAtFrame(FrameNow));
	}
}

void SXXAnimRootEditorDetails::WidgetPlayStateChanged(ECheckBoxState InState) {
	if (Viewport) {
		Viewport->SetPlaying(ECheckBoxState::Checked == InState);
	}
}

FReply SXXAnimRootEditorDetails::WidgetPlayBack() {
	ManualPause();
	if (FrameNow > 0) {
		SetCurrentFrame(FrameNow - 1);
		if (Viewport) {
			Viewport->SetPlayTime(CurrentAnimAsset->GetTimeAtFrame(FrameNow));
		}
	}
	return FReply::Handled();
}

FReply SXXAnimRootEditorDetails::WidgetPlayForward() {
	ManualPause();
	if (FrameNow < FrameCount) {
		SetCurrentFrame(FrameNow + 1);
		if (Viewport) {
			Viewport->SetPlayTime(CurrentAnimAsset->GetTimeAtFrame(FrameNow));
		}
	}
	return FReply::Handled();
}

FReply SXXAnimRootEditorDetails::WidgetSetBeginFrame() {
	FrameStart = FrameNow;
	FrameEnd = FMath::Max(FrameEnd, FrameStart);
	return FReply::Handled();
}

FReply SXXAnimRootEditorDetails::WidgetSetEndFrame() {
	FrameEnd = FrameNow;
	FrameStart = FMath::Min(FrameStart, FrameEnd);
	return FReply::Handled();
}

// The parent panel
SXXAnimRootEditorUI::SXXAnimRootEditorUI() : CurrentAnimAsset(nullptr), ContentBroserModule(nullptr){}

SXXAnimRootEditorUI::~SXXAnimRootEditorUI(){
	ContentBroserModule->GetOnAssetSelectionChanged().RemoveAll(this);
	AssetRegistryModule->Get().OnAssetRemoved().RemoveAll(this);
}

void SXXAnimRootEditorUI::Construct(const FArguments& InArgs) {
	ContentBroserModule = InArgs._ContentBrowser.Get();
	AssetRegistryModule = InArgs._AssetRegistry.Get();
	check(ContentBroserModule);
	// Binding selecting asset event
	ContentBroserModule->GetOnAssetSelectionChanged().AddRaw(this, &SXXAnimRootEditorUI::OnSelectedAssetsChanged);
	// Binding asset removing asset
	AssetRegistryModule->Get().OnAssetRemoved().AddRaw(this, &SXXAnimRootEditorUI::OnAssetRemoved);
	// Update selected animation sequence
	TArray<FAssetData> SelectedAssets;
	ContentBroserModule->Get().GetSelectedAssets(SelectedAssets);
	UpdateSelectedAnimAsset(SelectedAssets);
}

void SXXAnimRootEditorUI::SetAsset(UAnimSequence* Asset) {
	UAnimSequence* PrevAnimAsset = CurrentAnimAsset;
	CurrentAnimAsset = Asset;
	// null asset
	if (!CurrentAnimAsset) {
		
		ChildSlot[SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[
			SNew(STextBlock).Text(LOCTEXT("AnimAssetIsNull", "Please select a animation sequence!"))]];
		return;
	}

	// We need not recreate widgets if previous asset is not none.
	if (nullptr == PrevAnimAsset) {
		TSharedPtr<SXXAnimRootEditorViewport> Viewport;
		SAssignNew(WidgetDetails, SXXAnimRootEditorDetails);
		ChildSlot[
			SNew(SSplitter).Orientation(EOrientation::Orient_Horizontal)
				+ SSplitter::Slot().Value(0.66f).SizeRule(SSplitter::FractionOfParent)[
					SAssignNew(Viewport, SXXAnimRootEditorViewport)
				]
				+ SSplitter::Slot().Value(0.33f).SizeRule(SSplitter::FractionOfParent)[
					WidgetDetails.ToSharedRef()
				]
		];
		WidgetDetails->SetupViewport(Viewport->GetViewportClientOverride());
	}
	WidgetDetails->SetAsset(CurrentAnimAsset);
}

void SXXAnimRootEditorUI::UpdateSelectedAnimAsset(const TArray<FAssetData>& SelectedAssets) {
	UAnimSequence* Asset = nullptr;
	if (1 == SelectedAssets.Num()) {
		const FAssetData& SelectedAsset = SelectedAssets[0];
		if (UAnimSequence::StaticClass()->GetClassPathName() == SelectedAsset.AssetClassPath) {
			if (UAnimSequence* AnimAsset = Cast<UAnimSequence>(SelectedAsset.GetAsset())) {
				Asset = AnimAsset;
			}
		}
	}
	SetAsset(Asset);
}

void SXXAnimRootEditorUI::OnSelectedAssetsChanged(const TArray<FAssetData>& SelectedAssets, bool bIsPrimary) {
	if (bIsPrimary) {
		UpdateSelectedAnimAsset(SelectedAssets);
	}
}

inline bool IsAssetDataEquasAnimSequence(const FAssetData& AssetData, UAnimSequence* AnimSequence) {
	if(AssetData.IsAssetLoaded()) {
		return AssetData.GetAsset() == AnimSequence;
	}
	const FString AssetDataPath = AssetData.GetObjectPathString();
	const FString AnimSequencePath = AnimSequence->GetPathName();
	return AssetDataPath == AnimSequencePath;
}

void SXXAnimRootEditorUI::OnAssetRemoved(const FAssetData& AssetData) {
	if(IsAssetDataEquasAnimSequence(AssetData, CurrentAnimAsset)) {
		SetAsset(nullptr);
	}
}

#undef LOCTEXT_NAMESPACE
