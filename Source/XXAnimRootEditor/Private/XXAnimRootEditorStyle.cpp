#include "XXAnimRootEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FXXAnimRootEditorStyle::StyleInstance = nullptr;

void FXXAnimRootEditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FXXAnimRootEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FXXAnimRootEditorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("XXAnimRootEditorStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon24x24(16.0f, 16.0f);
const FVector2D Icon32x32(32.0f, 32.0f);

TSharedRef< FSlateStyleSet > FXXAnimRootEditorStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("XXAnimRootEditorStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("XXAnimRootEditor")->GetBaseDir() / TEXT("Resources"));

	Style->Set("XXAnimRootEditor.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon16x16));
	Style->Set("XXAnimRootEditor.Play", new IMAGE_BRUSH_SVG(TEXT("Play"), Icon24x24));
	Style->Set("XXAnimRootEditor.Pause", new IMAGE_BRUSH_SVG(TEXT("Pause"), Icon24x24));
	Style->Set("XXAnimRootEditor.Left", new IMAGE_BRUSH_SVG(TEXT("Left"), Icon24x24));
	Style->Set("XXAnimRootEditor.Right", new IMAGE_BRUSH_SVG(TEXT("Right"), Icon24x24));


	return Style;
}

void FXXAnimRootEditorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FXXAnimRootEditorStyle::Get()
{
	return *StyleInstance;
}
