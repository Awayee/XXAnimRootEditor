#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

// Combo of abitrray num of check box
class SMultiFlagsCheckBox : public SCompoundWidget {
public:
	SLATE_BEGIN_ARGS(SMultiFlagsCheckBox) : _NumItem(1), _IsChecked(false) {}
		SLATE_ARGUMENT(FText, Label);
		SLATE_ARGUMENT(uint8, NumItem);
		SLATE_ARGUMENT(bool, IsChecked);
	SLATE_END_ARGS();

	SMultiFlagsCheckBox();
	void Construct(const FArguments& InArgs);
	void SetItemText(int i, FText InText);
	bool IsItemChecked(int i) const;
	void SetAllChecked(bool bChecked);
	uint32 GetFlags() const;
private:
	// Determin whether the widget checked.
	uint32 AllFlags;
	uint32 CheckFlags;
	TSharedPtr<SCheckBox> Header;
	TArray<TSharedPtr<SCheckBox>> ChildCheckBoxes;
	TArray<TSharedPtr<STextBlock>> ChildTextBlocks;

	ECheckBoxState GetIsChecked() const;
};