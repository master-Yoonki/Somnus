// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Styling/SlateColor.h"
#include "SomnusDragDropVisualDataAsset.generated.h"

/**
 * What would happen if the drag payload were released on the hovered target right now.
 * Success can be placed as-is, Fail is a legal target that currently refuses this
 * placement (no room, occupied), Invalid is not a target for this payload at all (wrong
 * slot type, dropping a container into its own storage).
 */
UENUM(BlueprintType)
enum class EDragHoverState : uint8
{
	Success,
	Fail,
	Invalid
};

/**
 * One hover state's worth of highlight color, tinted by its own opacity rather than the
 * widget's overall alpha - keeps every state readable at a glance without a separate
 * opacity binding per state in UMG.
 */
USTRUCT(BlueprintType)
struct FSomnusDragHoverStyle
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drag Drop")
	FSlateColor Color = FSlateColor(FLinearColor::White);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drag Drop", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Opacity = 1.f;
};

/**
 * Highlight colors for hovering a drag payload over a drop target, one FSomnusDragHoverStyle
 * per EDragHoverState. Defaults are Tarkov's.
 */
UCLASS(BlueprintType, Const)
class SOMNUS_API USomnusDragDropVisualDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	USomnusDragDropVisualDataAsset();

	/** The style for State, or a flat white/opaque fallback (plus a warning log) when this
	 *  asset was never given an entry for it - lets a single OnDrag/OnDrop handler branch
	 *  on EDragHoverState and call this once instead of switching over three named fields. */
	UFUNCTION(BlueprintPure, Category = "Drag Drop")
	FSomnusDragHoverStyle GetStyle(EDragHoverState State) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Drag Drop")
	TMap<EDragHoverState, FSomnusDragHoverStyle> Styles;
};
