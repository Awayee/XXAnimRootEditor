#pragma once
#include "CoreMinimal.h"

class FToolBarBuilder;
class FMenuBuilder;
class SDockTab;
class FUICommandList;
class FSpawnTabArgs;
class FContentBrowserModule;
class FAssetRegistryModule;
class UAnimSequence;

class FXXAnimRootEditorApp {
public:
	FXXAnimRootEditorApp();
	~FXXAnimRootEditorApp();

private:
	TSharedPtr<FUICommandList> PluginCommands;
	FContentBrowserModule* ContentBroserModule;
	FAssetRegistryModule* AssetRegistryModule;
	
	void PluginButtonClicked(); // Bring up plugin window
	void RegisterMenus();
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);
};