#include "SMultiFlagsCheckBox.h"

SMultiFlagsCheckBox::SMultiFlagsCheckBox() : AllFlags(0), CheckFlags(0) {
}

void SMultiFlagsCheckBox::Construct(const FArguments& InArgs) {
	const uint8 NumItem = InArgs._NumItem;
	check(NumItem < 32);
	AllFlags = (1 << NumItem) - 1;
	CheckFlags = InArgs._IsChecked ? AllFlags : 0;
	Header = SNew(SCheckBox)
		.IsChecked_Raw(this, &SMultiFlagsCheckBox::GetIsChecked)
		.OnCheckStateChanged_Lambda([this](ECheckBoxState InState) {SetAllChecked(ECheckBoxState::Checked == InState); })
		.Content()[
			SNew(STextBlock).Text(InArgs._Label)
		];

	ChildTextBlocks.Reserve(NumItem);
	ChildCheckBoxes.Reserve(NumItem);
	TSharedPtr<SHorizontalBox> HorizontalBox;
	SAssignNew(HorizontalBox, SHorizontalBox) + SHorizontalBox::Slot()[Header.ToSharedRef()];
	for (uint8 i = 0; i < NumItem; ++i) {
		auto& TextBlock = ChildTextBlocks.AddDefaulted_GetRef();
		auto& CheckBox = ChildCheckBoxes.AddDefaulted_GetRef();
		SAssignNew(TextBlock, STextBlock);
		SAssignNew(CheckBox, SCheckBox)
		.IsChecked(CheckFlags & (1 << i) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
		.OnCheckStateChanged_Lambda([this, i](ECheckBoxState InState) {
			if (InState == ECheckBoxState::Checked) {
				this->CheckFlags |= (1 << i);
			}
			else {
				this->CheckFlags ^= (1 << i);
			}
			})
			.Content()[TextBlock.ToSharedRef()];
		HorizontalBox->AddSlot()[CheckBox.ToSharedRef()];
	}
	ChildSlot[HorizontalBox.ToSharedRef()];
}

void SMultiFlagsCheckBox::SetItemText(int i, FText InText) {
	check(i < ChildTextBlocks.Num());
	ChildTextBlocks[i]->SetText(InText);
}

bool SMultiFlagsCheckBox::IsItemChecked(int i) const {
	check(i < ChildCheckBoxes.Num());
	return ChildCheckBoxes[i]->IsChecked();
}

void SMultiFlagsCheckBox::SetAllChecked(bool bChecked) {
	for (auto& CheckBox : ChildCheckBoxes) {
		CheckBox->SetIsChecked(bChecked);
	}
	CheckFlags = bChecked ? AllFlags : 0u;
}

uint32 SMultiFlagsCheckBox::GetFlags() const {
	return CheckFlags;
}

ECheckBoxState SMultiFlagsCheckBox::GetIsChecked() const {
	if (AllFlags == CheckFlags) {
		return ECheckBoxState::Checked;
	}
	else if (0 == CheckFlags) {
		return ECheckBoxState::Unchecked;
	}
	return ECheckBoxState::Undetermined;
}

