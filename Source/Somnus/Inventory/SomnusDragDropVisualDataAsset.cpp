// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/SomnusDragDropVisualDataAsset.h"

USomnusDragDropVisualDataAsset::USomnusDragDropVisualDataAsset()
{
	// Tarkov's own hover colors: green fits, red is legal but refused right now, yellow is
	// not a valid target for this payload at all.
	Styles.Add(EDragHoverState::Success, FSomnusDragHoverStyle{ FSlateColor(FLinearColor(0.f, 1.f, 0.f)), 1.f });
	Styles.Add(EDragHoverState::Fail, FSomnusDragHoverStyle{ FSlateColor(FLinearColor(1.f, 0.f, 0.f)), 1.f });
	Styles.Add(EDragHoverState::Invalid, FSomnusDragHoverStyle{ FSlateColor(FLinearColor(1.f, 0.85f, 0.f)), 1.f });
}

FSomnusDragHoverStyle USomnusDragDropVisualDataAsset::GetStyle(EDragHoverState State) const
{
	if (const FSomnusDragHoverStyle* Found = Styles.Find(State))
	{
		return *Found;
	}

	UE_LOG(LogTemp, Warning, TEXT("USomnusDragDropVisualDataAsset::GetStyle: %s has no entry for %s, falling back to opaque white."),
		*GetNameSafe(this), *UEnum::GetValueAsString(State));
	return FSomnusDragHoverStyle();
}
