#include "XXAnimRootEditor.h"
#include "XXAnimRootEditorApp.h"


DEFINE_LOG_CATEGORY(XXAnimRootEditor);

void FXXAnimRootEditorModule::StartupModule()
{
	XXAnimRootEditorApp.Reset(new FXXAnimRootEditorApp());
}

void FXXAnimRootEditorModule::ShutdownModule()
{
	XXAnimRootEditorApp.Reset();
}
	
IMPLEMENT_MODULE(FXXAnimRootEditorModule, XXAnimRootEditor)