// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SomnusHitTraceTestHUD.generated.h"

// Draws a small dot at the center of the viewport so you can see where
// ASomnusHitTraceTestCharacter is aiming. Set as the HUD Class override in
// World Settings -> Selected GameMode for the test level.
UCLASS()
class SOMNUS_API ASomnusHitTraceTestHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	UPROPERTY(EditAnywhere, Category = "Hit Trace Test")
	float DotSize = 10.f;

	// Bright, saturated default so it reads against most backgrounds; the black
	// outline drawn behind it does the rest of the contrast work.
	UPROPERTY(EditAnywhere, Category = "Hit Trace Test")
	FLinearColor DotColor = FLinearColor(0.f, 1.f, 0.2f);

	UPROPERTY(EditAnywhere, Category = "Hit Trace Test")
	float OutlineThickness = 2.f;

	UPROPERTY(EditAnywhere, Category = "Hit Trace Test")
	FLinearColor OutlineColor = FLinearColor::Black;
};
