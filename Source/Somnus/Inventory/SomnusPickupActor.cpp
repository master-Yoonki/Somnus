// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/SomnusPickupActor.h"

#include "SomnusContainerActor.h"
#include "Inventory/SomnusContainerEquipComponent.h"
#include "Inventory/SomnusEquipmentComponent.h"
#include "SomnusItemDataAsset.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

// Sets default values
ASomnusPickupActor::ASomnusPickupActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	RootComponent = PickupMesh;
	PickupMesh->SetCollisionProfileName(TEXT("Pickup"));
	// A body that cannot move cannot simulate, and SetSimulatePhysics only warns about it.
	PickupMesh->SetMobility(EComponentMobility::Movable);
	// Loot is small and floors are thin. Without continuous detection one long frame integrates
	// the body clean past the ground and no contact is ever generated to stop it.
	PickupMesh->BodyInstance.bUseCCD = true;

	// The server's body is the authority on where loot came to rest; without this every client
	// settles it slightly differently and they never reconcile.
	SetReplicateMovement(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultFallbackMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultFallbackMesh.Succeeded())
	{
		FallbackMesh = DefaultFallbackMesh.Object;
	}

#if WITH_EDITORONLY_DATA
	EditorVisualizer = CreateDefaultSubobject<USphereComponent>(TEXT("EditorVisualizer"));
	EditorVisualizer->SetupAttachment(PickupMesh);
	EditorVisualizer->bIsEditorOnly = true;
	EditorVisualizer->SetSphereRadius(50.f);
	EditorVisualizer->ShapeColor = FColor(255, 190, 40);
	// It exists to be looked at. Anything else it touched would be a trap - the interact trace
	// runs against a channel this would answer for otherwise.
	EditorVisualizer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
#endif
}

void ASomnusPickupActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASomnusPickupActor, PickupItem);
}

void ASomnusPickupActor::Interact_Implementation(AActor* Interactor)
{
	PickUp(Interactor);
}

void ASomnusPickupActor::SetHighlighted_Implementation(bool bHighlighted)
{
	PickupMesh->SetCustomDepthStencilValue(bHighlighted ? SomnusStencil::Interactable : SomnusStencil::None);
	PickupMesh->SetRenderCustomDepth(bHighlighted);
}

void ASomnusPickupActor::InitializeFromInstance(const FSomnusItemInstance* Instance)
{
	if (!Instance) return;
	if (!HasAuthority()) return;
	
	PickupItem = *Instance;
	if (PickupItem.ContainerActor)
	{
		PickupItem.ContainerActor->SetOwner(this);
	}
	RefreshPickupMesh();
}

bool ASomnusPickupActor::TryWearItem(AActor* Interactor)
{
	// Weapons first, then storage. The two never compete - what a slot admits is a tag, and no item
	// carries both a weapon tag and a container one - so the order between them costs nothing and
	// only spares a second lookup for the common case.
	if (USomnusEquipmentComponent* Equipment = Interactor->GetComponentByClass<USomnusEquipmentComponent>())
	{
		if (Equipment->EquipInstance(PickupItem))
		{
			return true;
		}
	}

	if (USomnusContainerEquipComponent* Storage = Interactor->GetComponentByClass<USomnusContainerEquipComponent>())
	{
		return Storage->EquipInstance(PickupItem);
	}

	return false;
}

bool ASomnusPickupActor::PickUp(AActor* Interactor)
{
	if (!HasAuthority() || !Interactor || !PickupItem.InstanceID.IsValid()) return false;

	USomnusContainerEquipComponent* ContainerEquipComponent = Interactor->GetComponentByClass<USomnusContainerEquipComponent>();
	if (!ContainerEquipComponent) return false;

	// Worn before stored. Reaching for a pack means putting it on, and burying it inside the pack
	// already on your back is the fallback, not the intent. Each component is asked in turn rather
	// than every slot on the actor at once, because the order within one is a decision it owns -
	// a gun with both hands free belongs in the primary, and GetComponents has no opinion on that.
	//
	// A single item only. A slot holds one thing, so a stack landing in one would put a count
	// somewhere with no room to express it.
	if (PickupItem.StackCount == 1 && TryWearItem(Interactor))
	{
		// Storage follows its holder, and this pickup is about to stop existing. EquipInstance has
		// already re-owned any container actor it placed; this covers the paths that did not.
		if (PickupItem.ContainerActor)
		{
			PickupItem.ContainerActor->SetOwner(Interactor);
		}

		Destroy();
		return true;
	}

	// Read the count first: TryAddExistingItemAnywhere consumes PickupItem in place, so this is
	// the only chance to tell a partial take from an outright refusal once it returns.
	const int32 QuantityBefore = PickupItem.StackCount;
	const int32 Leftover = ContainerEquipComponent->TryAddExistingItemAnywhere(PickupItem);

	if (Leftover >= QuantityBefore)
	{
		return false;   // no room anywhere, so the world keeps everything
	}

	if (Leftover > 0)
	{
		// A partial take. PickupItem already holds the reduced count and replicates it out on
		// its own, so the remainder simply stays standing where it was.
		return true;
	}

	// Storage follows its holder so client RPCs keep routing and relevancy keeps resolving, and
	// this pickup is about to stop existing. The container actor itself is never destroyed - it
	// is the contents of the item that just entered the inventory.
	if (PickupItem.ContainerActor)
	{
		PickupItem.ContainerActor->SetOwner(Interactor);
	}

	Destroy();
	return true;
}

void ASomnusPickupActor::RefreshPickupMesh()
{
	// PickupItem wins wherever it exists, but it is minted at BeginPlay - which means it is
	// empty for the whole time the actor sits in a level being placed and arranged. There
	// SpawnItemData is the only description of what this pickup is going to be.
	USomnusItemDataAsset* SourceData = PickupItem.ItemData ? PickupItem.ItemData : SpawnItemData;

	UStaticMesh* MeshToUse = SourceData ? SourceData->WorldMesh : nullptr;
	if (!MeshToUse)
	{
		MeshToUse = FallbackMesh;
	}

	PickupMesh->SetStaticMesh(MeshToUse);
}

void ASomnusPickupActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Reruns on every edit to the details panel, so picking a SpawnItemData shows that item's
	// mesh in the viewport immediately rather than at the next PIE.
	RefreshPickupMesh();
}

void ASomnusPickupActor::OnRep_PickupItem()
{
	RefreshPickupMesh();
}

// Called when the game starts or when spawned
void ASomnusPickupActor::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		if (!PickupItem.InstanceID.IsValid())
		{
			if (SpawnItemData && SpawnQuantity > 0)
			{
				PickupItem = FSomnusItemInstance::MakeItemInstance(GetWorld(), SpawnItemData.Get(), SpawnQuantity);
			}
		}
		if (PickupItem.ContainerActor)
		{
			PickupItem.ContainerActor->SetOwner(this);
		}
	}

	// Unconditional, and deliberately duplicating what OnRep_PickupItem does: the server has no
	// OnRep to lean on, and a client whose PickupItem arrived before BeginPlay ran has already
	// spent its one. Setting the same mesh twice costs nothing.
	RefreshPickupMesh();

	// Deferred a tick rather than started here. BeginPlay runs while the world is still coming
	// up, and loot placed in a level was falling through the floor whenever loading ran long:
	// a body turned loose before the ground it stands on is collidable has nothing to land on.
	// By the next tick every actor has begun play and its physics state is registered.
	GetWorldTimerManager().SetTimerForNextTick(this, &ASomnusPickupActor::BeginPhysicsSimulation);
}

void ASomnusPickupActor::BeginPhysicsSimulation()
{
	// Only now, because assigning a static mesh rebuilds the physics body from scratch. Loot is
	// dropped rather than placed to the centimetre, so it drops and settles from here on - in
	// the editor it stays exactly where it was put, which is the whole point of placing it.
	PickupMesh->SetSimulatePhysics(true);
}