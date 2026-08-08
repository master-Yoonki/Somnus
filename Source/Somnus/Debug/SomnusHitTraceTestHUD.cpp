// Fill out your copyright notice in the Description page of Project Settings.

#include "Debug/SomnusHitTraceTestHUD.h"
#include "Engine/Canvas.h"

void ASomnusHitTraceTestHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const float CenterX = Canvas->SizeX * 0.5f;
	const float CenterY = Canvas->SizeY * 0.5f;

	const float OutlineSize = DotSize + OutlineThickness * 2.f;
	DrawRect(OutlineColor, CenterX - OutlineSize * 0.5f, CenterY - OutlineSize * 0.5f, OutlineSize, OutlineSize);
	DrawRect(DotColor, CenterX - DotSize * 0.5f, CenterY - DotSize * 0.5f, DotSize, DotSize);
}
