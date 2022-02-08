// Copyright (c) Mathew Wang 2021

#pragma once
#include "hIKEditor.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "AnimNode_FKRecordUT.h"
#include "AnimGraphNode_FKRecordUT.generated.h"

UCLASS()
class HIKEDITOR_API UAnimGraphNode_FKRecordUT : public UAnimGraphNode_Base
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = HIKAnim)
	FAnimNode_FKRecordUT Node;

	//~ Begin UEdGraphNode Interface.
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UEdGraphNode Interface.

	//~ Begin UAnimGraphNode_Base Interface
	virtual FString GetNodeCategory() const override;
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const override;
	//~ End UAnimGraphNode_Base Interface
};