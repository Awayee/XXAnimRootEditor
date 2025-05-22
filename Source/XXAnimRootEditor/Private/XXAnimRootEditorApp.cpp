#include "XXAnimRootEditorApp.h"
#include "XXAnimRootEditorStyle.h"
#include "XXAnimRootEditorUI.h"
#include "ContentBrowserModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/Commands/Commands.h"

#define LOCTEXT_NAMESPACE "FXXAnimRootEditorModule"
static const FName XXAnimRootEditorTabId("XXAnimRootEditor");

class FXXAnimRootEditorCommands : public TCommands<FXXAnimRootEditorCommands> {
public:
	TSharedPtr<FUICommandInfo> OpenPluginWindow;
	FXXAnimRootEditorCommands() :
		TCommands<FXXAnimRootEditorCommands>(TEXT("XXAnimRootEditor"), NSLOCTEXT("Contexts", "XXAnimRootEditor", "XXAnimRootEditor Plugin"), NAME_None, FXXAnimRootEditorStyle::GetStyleSetName())
	{}
	virtual void RegisterCommands() override {
		UI_COMMAND(OpenPluginWindow, "XXAnimRootEditor", "Bring up XXAnimRootEditor window", EUserInterfaceActionType::Button, FInputChord());
	}
};

FXXAnimRootEditorApp::FXXAnimRootEditorApp(): ContentBroserModule(nullptr){
	FXXAnimRootEditorStyle::Initialize();
	FXXAnimRootEditorStyle::ReloadTextures();
	FXXAnimRootEditorCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList());
	PluginCommands->MapAction(FXXAnimRootEditorCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FXXAnimRootEditorApp::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FXXAnimRootEditorApp::RegisterMenus));
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(XXAnimRootEditorTabId, FOnSpawnTab::CreateRaw(this, &FXXAnimRootEditorApp::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("XXAnimRootEditorTabSpawn", "XXAnimRootEditor"))
		.SetMenuType(ETabSpawnerMenuType::Type::Hidden);

	// Initialize content browser
	ContentBroserModule = &FModuleManager::GetModuleChecked<FContentBrowserModule>("ContentBrowser");
	AssetRegistryModule = &FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	check(ContentBroserModule);
	check(AssetRegistryModule);
}

FXXAnimRootEditorApp::~FXXAnimRootEditorApp() {
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FXXAnimRootEditorStyle::Shutdown();
	FXXAnimRootEditorCommands::Unregister();
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(XXAnimRootEditorTabId);
}

void FXXAnimRootEditorApp::PluginButtonClicked() {
	FGlobalTabmanager::Get()->TryInvokeTab(XXAnimRootEditorTabId);
}

void FXXAnimRootEditorApp::RegisterMenus() {
	FToolMenuOwnerScoped OwnerScoped((void*)this);
	FSlateIcon ButtonIcon{ FXXAnimRootEditorStyle::GetStyleSetName(), "XXAnimRootEditor.OpenPluginWindow" };
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		FToolMenuSection& Section = Menu->FindOrAddSection(NAME_None);
		FToolMenuEntry& Entry = Section.AddMenuEntryWithCommandList(FXXAnimRootEditorCommands::Get().OpenPluginWindow, PluginCommands);
		Entry.Icon = ButtonIcon;
	}
	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.User");
		FToolMenuSection& Section = ToolbarMenu->FindOrAddSection(NAME_None);
		FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FXXAnimRootEditorCommands::Get().OpenPluginWindow));
		Entry.SetCommandList(PluginCommands);
		Entry.Icon = ButtonIcon;
	}
}

TSharedRef<SDockTab> FXXAnimRootEditorApp::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs) {
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)[
			SNew(SXXAnimRootEditorUI).ContentBrowser(ContentBroserModule).AssetRegistry(AssetRegistryModule)
		];
}

#undef LOCTEXT_NAMESPACE