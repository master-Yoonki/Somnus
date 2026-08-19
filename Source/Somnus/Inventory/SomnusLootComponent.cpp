// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusLootComponent.h"

#include "AbilitySystemComponent.h"
#include "SomnusContainerActor.h"
#include "SomnusContainerEquipComponent.h"
#include "SomnusEquipmentSlotComponent.h"
#include "SomnusInventoryComponent.h"
#include "SomnusItemDataAsset.h"
#include "Character/SomnusCharacter.h"
#include "Core/SomnusGameplayTags.h"
#include "Net/UnrealNetwork.h"

USomnusLootComponent::USomnusLootComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// LootTarget is the whole point of this component and it lives on the client's screen, so
	// without this nothing downstream of it exists on the client at all.
	SetIsReplicatedByDefault(true);
}

void USomnusLootComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION(USomnusLootComponent, LootTarget, COND_OwnerOnly);
}

void USomnusLootComponent::OpenLoot(AActor* Body)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!IsWithinReach(Body)) return;

	SetLootTarget(Body);
}

void USomnusLootComponent::SetLootTarget(AActor* Body)
{
	// The equality check is what keeps the hand call below honest. A client only ever hears about
	// a real change, because replication has nothing to send when the value did not move; without
	// this the authority would announce closings that never happened, and a listen server host
	// would be the one player whose panel opens as they die.
	if (LootTarget == Body)
	{
		return;
	}

	LootTarget = Body;
	OnRep_LootTarget();
}

bool USomnusLootComponent::IsWithinReach(const AActor* Body) const
{
	if (!Body || !GetOwner()) return false;

	return FVector::DistSquared(GetOwner()->GetActorLocation(), Body->GetActorLocation())
		< FMath::Square(LootRange);
}

bool USomnusLootComponent::CanAccessHolder(const AActor* Holder) const
{
	if (!Holder || !GetOwner()) return false;

	if (Holder == GetOwner()) return true;
	if (Holder == LootTarget) return IsWithinReach(Holder);

	return false;
}

bool USomnusLootComponent::CanAccessContainer(const class USomnusInventoryComponent* Container) const
{
	// A grid answers for whoever is ultimately holding it, which is the only level the question
	// has an answer at - containers nest, and each link owns the next.
	return Container && CanAccessHolder(ASomnusContainerActor::ResolveRootHolder(Container->GetOwner()));
}

void USomnusLootComponent::Server_UseItem_Implementation(USomnusInventoryComponent* Source, FGuid InstanceID)
{    
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!Source) return;
	if (!CanAccessContainer(Source)) return;
	
	const FSomnusItemInstance* Instance = Source->FindItemInstance(InstanceID);
	if (!Instance || !Instance->ItemData) return;
	FGameplayTag ItemTag = Instance->ItemData->ItemTag;
	if (!ItemTag.IsValid()) return;
	if (!ItemTag.MatchesTag(SomnusTags::Item_Consumable)) return;
	if (Instance->ItemData->UseEffects.IsEmpty()) return;
	
	ASomnusCharacter* Character = Cast<ASomnusCharacter>(GetOwner());
	if (!Character) return;
	
	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC) return;
	
	bool bAnySucceed = false;
	for (const TSubclassOf<UGameplayEffect>& EffectClass : Instance->ItemData->UseEffects)
	{
		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddSourceObject(Instance->ItemData);
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.f, ContextHandle);
		
		if (ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get()).IsValid())
		{
			bAnySucceed = true;
		}
	}
	
	if (!bAnySucceed) return;

	Source->ConsumeOneFromStack(InstanceID);
}

void USomnusLootComponent::Server_MoveItem_Implementation(USomnusInventoryComponent* Source,
                                                          USomnusInventoryComponent* Dest, FGuid InstanceID, int32 TopLeftX, int32 TopLeftY, bool bRotated)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!Source || !Dest) return;
	
	if (!CanAccessContainer(Source)) return;
	if (!CanAccessContainer(Dest)) return;

	// A locked slot keeps what it holds. Checked here rather than in the grid, because what a
	// player may ask for is the whole question - the server still empties a corpse and grants a
	// starting kit through the same components.
	if (const USomnusEquipmentSlotComponent* Slot = Cast<USomnusEquipmentSlotComponent>(Source))
	{
		if (Slot->IsLocked()) return;
	}

	Dest->MoveItemFrom(Source, InstanceID, TopLeftX, TopLeftY, bRotated);
}

void USomnusLootComponent::Server_CloseLoot_Implementation()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	SetLootTarget(nullptr);
}

void USomnusLootComponent::Server_DropFrom_Implementation(USomnusContainerEquipComponent* Source,
	FGuid InstanceID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!Source || !Source->GetOwner()) return;
	if (!CanAccessHolder(Source->GetOwner())) return;
	
	Source->DropItem(InstanceID);
}

void USomnusLootComponent::OnRep_LootTarget()
{
	// Called by hand from the two server-side writers as well, because OnRep never fires on the
	// authority - a listen server host would otherwise be the one player whose panel never opens.
	OnLootTargetChanged.Broadcast();
}


