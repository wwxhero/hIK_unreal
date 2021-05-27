// Copyright (c) Mathew Wang 2021

#pragma once
#include "CoreMinimal.h"
#include "Engine.h"
#include "Modules/ModuleManager.h"
#include "UnrealEd.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHIKEditor, All, All)

class FhIKEditorModule
	: public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

};
