// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"


DECLARE_LOG_CATEGORY_EXTERN(XXAnimRootEditor, Log, All)

class FXXAnimRootEditorApp;

class FXXAnimRootEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
private:
	TUniquePtr<FXXAnimRootEditorApp> XXAnimRootEditorApp;
};
