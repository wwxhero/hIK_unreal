// Copyright (c) Mathew Wang 2021

#include "hIKEditor.h"

IMPLEMENT_GAME_MODULE(FhIKEditorModule, hIKEditor);

DEFINE_LOG_CATEGORY(LogHIKEditor)

void FhIKEditorModule::StartupModule()
{
	UE_LOG(LogHIKEditor, Warning, TEXT("IK editor module staring"));
}

void FhIKEditorModule::ShutdownModule()
{
	UE_LOG(LogHIKEditor, Warning, TEXT("IK editor module shutdown"));
}

