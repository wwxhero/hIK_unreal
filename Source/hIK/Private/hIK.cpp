// Copyright Epic Games, Inc. All Rights Reserved.
//#include "stdafx.h"
#include "hIK.h"

#define LOCTEXT_NAMESPACE "FhIKModule"

DEFINE_LOG_CATEGORY(LogHIK)

void FhIKModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FhIKModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FhIKModule, hIK)