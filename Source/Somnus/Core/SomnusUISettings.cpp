// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/SomnusUISettings.h"

USomnusUISettings::USomnusUISettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Somnus UI");
}

USomnusUISettings* USomnusUISettings::Get()
{
	// The class default object is the settings object - there is never a second one, and the
	// config values are loaded into it before anything asks. Mutable only because a UFUNCTION
	// cannot hand Blueprint a const pointer; every field is BlueprintReadOnly, so nothing can
	// write through it from there.
	return GetMutableDefault<USomnusUISettings>();
}
