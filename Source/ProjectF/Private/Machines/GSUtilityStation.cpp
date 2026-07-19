#include "Machines/GSUtilityStation.h"
#include "Components/BoxComponent.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "Core/GSGameplayTags.h"
#include "DataAssets/GSStationDataAsset.h"
#include "Items/GSItem.h"
#include "Components/GSGrabbableComponent.h"
#include "DataAssets/GSItemDataAsset.h"


AGSUtilityStation::AGSUtilityStation()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	StationVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("StationVolume"));
	RootComponent = StationVolume;
	StationVolume->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	StationVolume->SetCollisionProfileName(TEXT("Trigger"));
	StationVolume->SetGenerateOverlapEvents(true);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	bHidePlacedItems = true;
	bLimitToSockets = false;
}

UAbilitySystemComponent* AGSUtilityStation::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGSUtilityStation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGSUtilityStation, PlacedItems);
}

void AGSUtilityStation::OnRep_PlacedItems(const TArray<AActor*>& OldPlacedItems)
{
	for (AActor* Item : PlacedItems)
	{
		if (IsValid(Item) && !OldPlacedItems.Contains(Item))
		{
			HandleItemAddedToStation(Item);
		}
	}

	for (AActor* Item : OldPlacedItems)
	{
		if (IsValid(Item) && !PlacedItems.Contains(Item))
		{
			HandleItemRemovedFromStation(Item);
		}
	}
}

void AGSUtilityStation::HandleItemAddedToStation(AActor* Item)
{
	if (IsValid(Item))
	{

		if (bHidePlacedItems)
		{
			Item->SetActorHiddenInGame(true);
		}


		UPrimitiveComponent* PrimRoot = Cast<UPrimitiveComponent>(Item->GetRootComponent());
		if (!PrimRoot)
		{
			PrimRoot = Item->FindComponentByClass<UPrimitiveComponent>();
		}
		if (PrimRoot)
		{
			PrimRoot->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}


		if (HasAuthority())
		{
			FName FreeSocket = GetFirstFreeSocket();
			if (!FreeSocket.IsNone())
			{
				USceneComponent* AttachTarget = GetRootComponent();
				if (UMeshComponent* MeshComp = FindComponentByClass<UMeshComponent>())
				{
					AttachTarget = MeshComp;
				}

				if (AttachTarget)
				{
					Item->AttachToComponent(AttachTarget, FAttachmentTransformRules::SnapToTargetIncludingScale, FreeSocket);
				}
			}
			else
			{
				Item->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			}
		}


		AGSItem* GSItem = Cast<AGSItem>(Item);
		if (GSItem && GSItem->ItemData)
		{
			if (UAbilitySystemComponent* TargetASC = GSItem->GetAbilitySystemComponent())
			{
				TWeakObjectPtr<AActor> WeakItem = Item;


				if (StationSubscriptions.Contains(WeakItem))
				{
					return;
				}

				FGSItemSubscription& Sub = StationSubscriptions.FindOrAdd(WeakItem);

				for (const TPair<FGameplayTag, FGSItemStateDetails>& StatePair : GSItem->ItemData->ItemStatesMap)
				{
					const FGameplayTag& StateTag = StatePair.Key;
					FDelegateHandle Handle = TargetASC->RegisterGameplayTagEvent(StateTag, EGameplayTagEventType::NewOrRemoved)
						.AddUObject(this, &AGSUtilityStation::OnItemStateTagChanged, StateTag, WeakItem);

					Sub.TagHandles.Add(StateTag, Handle);
				}
			}
		}


		if (UGSGrabbableComponent* GrabComp = Item->FindComponentByClass<UGSGrabbableComponent>())
		{
			GrabComp->OnGrabbed.AddUniqueDynamic(this, &AGSUtilityStation::OnPlacedItemGrabbed);
		}


		UE_LOG(LogTemp, Log, TEXT("[STATION_DEBUG] OnItemAddedToStation: Item %s attached and hidden"), *Item->GetName());

		OnItemAddedToStation(Item);
	}
}

void AGSUtilityStation::HandleItemRemovedFromStation(AActor* Item)
{
	if (IsValid(Item))
	{
		UGSGrabbableComponent* GrabComp = Item->FindComponentByClass<UGSGrabbableComponent>();
		bool bIsGrabbed = GrabComp && GrabComp->IsGrabbed();

		if (!bIsGrabbed)
		{

			Item->SetActorHiddenInGame(false);


			Item->SetActorEnableCollision(true);


			if (Item->GetAttachParentActor() == this)
			{
				Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			}
		}
		else
		{

			Item->SetActorHiddenInGame(false);
		}


		TWeakObjectPtr<AActor> WeakItem = Item;
		FGSItemSubscription* SubPtr = StationSubscriptions.Find(WeakItem);
		if (SubPtr)
		{
			AGSItem* GSItem = Cast<AGSItem>(Item);
			if (GSItem)
			{
				if (UAbilitySystemComponent* TargetASC = GSItem->GetAbilitySystemComponent())
				{
					for (const TPair<FGameplayTag, FDelegateHandle>& HandlePair : SubPtr->TagHandles)
					{
						TargetASC->RegisterGameplayTagEvent(HandlePair.Key, EGameplayTagEventType::NewOrRemoved)
							.Remove(HandlePair.Value);
					}
				}
			}
			StationSubscriptions.Remove(WeakItem);
		}


		if (GrabComp)
		{
			GrabComp->OnGrabbed.RemoveDynamic(this, &AGSUtilityStation::OnPlacedItemGrabbed);
		}


		UE_LOG(LogTemp, Log, TEXT("[STATION_DEBUG] OnItemRemovedFromStation: Item %s detached and shown"), *Item->GetName());

		OnItemRemovedFromStation(Item);
	}
}

void AGSUtilityStation::BeginPlay()
{
	Super::BeginPlay();


	if (StationData)
	{
		EffectsToApply = StationData->StationDetails.EffectsToApply;
		AttributeSets = StationData->StationDetails.AttributeSets;
		ConditionalEffectsFromData = StationData->StationDetails.ConditionalEffects;

		if (StationVolume)
		{
			StationVolume->SetBoxExtent(StationData->StationDetails.HitBoxSize, true);
		}
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);


		for (const TSubclassOf<UAttributeSet>& SetClass : AttributeSets)
		{
			if (SetClass)
			{
				AbilitySystemComponent->InitStats(SetClass, nullptr);
			}
		}
	}


	if (StationVolume)
	{
		StationVolume->OnComponentBeginOverlap.AddDynamic(this, &AGSUtilityStation::OnOverlapBegin);
		StationVolume->OnComponentEndOverlap.AddDynamic(this, &AGSUtilityStation::OnOverlapEnd);
	}
}

AActor* AGSUtilityStation::GetLastPlacedItem() const
{
	for (int32 i = PlacedItems.Num() - 1; i >= 0; --i)
	{
		if (IsValid(PlacedItems[i]))
		{
			return PlacedItems[i];
		}
	}
	return nullptr;
}

AActor* AGSUtilityStation::RemoveLastPlacedItem()
{
	if (!HasAuthority())
	{
		return GetLastPlacedItem();
	}

	for (int32 i = PlacedItems.Num() - 1; i >= 0; --i)
	{
		AActor* Item = PlacedItems[i];
		if (IsValid(Item))
		{
			RemovePlacedItem(Item);
			return Item;
		}
		PlacedItems.RemoveAt(i);
	}
	return nullptr;
}

AActor* AGSUtilityStation::RemovePlacedItem(AActor* Item)
{
	if (!IsValid(Item)) { return nullptr; }

	if (HasAuthority())
	{

		if (TArray<FActiveGameplayEffectHandle>* Handles = AppliedEffectsMap.Find(Item))
		{
			if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Item))
			{
				if (UAbilitySystemComponent* TargetASC = ASCInterface->GetAbilitySystemComponent())
				{
					for (const FActiveGameplayEffectHandle& Handle : *Handles)
					{
						if (Handle.IsValid())
						{
							TargetASC->RemoveActiveGameplayEffect(Handle);
						}
					}
					UE_LOG(LogTemp, Warning, TEXT("[STATION_DEBUG] RemovePlacedItem: Removed %d GE handles from %s"), Handles->Num(), *Item->GetName());
				}
			}
			AppliedEffectsMap.Remove(Item);
		}



		HandleItemRemovedFromStation(Item);

		PlacedItems.Remove(Item);
	}

	return Item;
}

const TArray<AActor*>& AGSUtilityStation::GetPlacedItems() const
{
	return PlacedItems;
}

AActor* AGSUtilityStation::GetFirstReadyItem() const
{
	for (AActor* Item : PlacedItems)
	{
		if (IsValid(Item))
		{
			if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Item))
			{
				if (UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent())
				{

					if (ASC->HasMatchingGameplayTag(GSGameplayTags::State_Cooked) ||
						ASC->HasMatchingGameplayTag(GSGameplayTags::State_Cooled) ||
						ASC->HasMatchingGameplayTag(GSGameplayTags::State_Filled))
					{
						return Item;
					}
				}
			}
		}
	}
	return GetLastPlacedItem();
}

void AGSUtilityStation::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) { return; }

	if (OtherActor && OtherActor != this)
	{
		if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(OtherActor))
		{
			if (UAbilitySystemComponent* TargetASC = ASCInterface->GetAbilitySystemComponent())
			{

				if (AllowedItemTags.Num() > 0 && !TargetASC->HasAnyMatchingGameplayTags(AllowedItemTags))
				{
					return;
				}


				UGSGrabbableComponent* GrabComp = OtherActor->FindComponentByClass<UGSGrabbableComponent>();
				if (GrabComp && GrabComp->IsGrabbed())
				{
					return;
				}


				if (bLimitToSockets && GetFirstFreeSocket().IsNone())
				{
					return;
				}


				if (!PlacedItems.Contains(OtherActor))
				{
					PlacedItems.Add(OtherActor);


					HandleItemAddedToStation(OtherActor);


					UpdateEffectsForItem(OtherActor);
				}
			}
		}
	}
}

void AGSUtilityStation::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority()) { return; }

	if (OtherActor)
	{

		if (PlacedItems.Contains(OtherActor))
		{

			if (OtherActor->GetAttachParentActor() == this)
			{
				return;
			}

			RemovePlacedItem(OtherActor);
		}
	}
}

void AGSUtilityStation::OnItemStateTagChanged(const FGameplayTag Tag, int32 NewCount, FGameplayTag StateTag, TWeakObjectPtr<AActor> WeakItem)
{
	AActor* Item = WeakItem.Get();
	if (IsValid(Item))
	{
		if (HasAuthority())
		{
			UpdateEffectsForItem(Item);
		}


		OnAttachedItemStateChanged(Item, StateTag, NewCount > 0);
	}
}

void AGSUtilityStation::OnPlacedItemGrabbed(AActor* GrabbedItem)
{
	if (IsValid(GrabbedItem) && HasAuthority())
	{
		RemovePlacedItem(GrabbedItem);
	}
}

void AGSUtilityStation::ApplyEffectToItem(AActor* Item, TSubclassOf<UGameplayEffect> EffectClass)
{
	if (!HasAuthority() || !IsValid(Item) || !EffectClass) { return; }

	if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Item))
	{
		if (UAbilitySystemComponent* TargetASC = ASCInterface->GetAbilitySystemComponent())
		{
			FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
			EffectContext.AddInstigator(this, this);
			FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
			if (SpecHandle.IsValid())
			{
				FActiveGameplayEffectHandle Handle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
				if (Handle.IsValid())
				{

					AppliedEffectsMap.FindOrAdd(Item).Add(Handle);
					if (bShowDebugLogs)
					{
						UE_LOG(LogTemp, Warning, TEXT("[STATION] ApplyEffectToItem: Applied %s to %s"), *EffectClass->GetName(), *Item->GetName());
					}
				}
			}
		}
	}
}

FName AGSUtilityStation::GetFirstFreeSocket() const
{
	for (const FName& SocketName : StationSockets)
	{
		bool bIsOccupied = false;
		for (const AActor* PlacedItem : PlacedItems)
		{
			if (IsValid(PlacedItem) && PlacedItem->GetRootComponent())
			{
				if (PlacedItem->GetRootComponent()->GetAttachSocketName() == SocketName)
				{
					bIsOccupied = true;
					break;
				}
			}
		}

		if (!bIsOccupied)
		{
			return SocketName;
		}
	}
	return NAME_None;
}

void AGSUtilityStation::UpdateEffectsForItem(AActor* Item)
{
	if (!HasAuthority() || !IsValid(Item)) { return; }

	UAbilitySystemComponent* TargetASC = nullptr;
	if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Item))
	{
		TargetASC = ASCInterface->GetAbilitySystemComponent();
	}
	if (!TargetASC) { return; }


	TArray<FActiveGameplayEffectHandle>* ActiveHandles = AppliedEffectsMap.Find(Item);
	if (ActiveHandles)
	{
		for (const FActiveGameplayEffectHandle& Handle : *ActiveHandles)
		{
			if (Handle.IsValid())
			{
				TargetASC->RemoveActiveGameplayEffect(Handle);
			}
		}
		ActiveHandles->Empty();
	}
	else
	{
		ActiveHandles = &AppliedEffectsMap.Add(Item);
	}


	bool bAppliedConditional = false;
	for (const FGSConditionalEffectEntry& Entry : ConditionalEffectsFromData)
	{
		if (Entry.StateTag.IsValid() && TargetASC->HasMatchingGameplayTag(Entry.StateTag))
		{
			for (const TSubclassOf<UGameplayEffect>& EffectClass : Entry.EffectsToApply)
			{
				if (EffectClass)
				{
					FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
					EffectContext.AddInstigator(this, this);
					FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
					if (SpecHandle.IsValid())
					{
						FActiveGameplayEffectHandle Handle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
						if (Handle.IsValid())
						{
							ActiveHandles->Add(Handle);
							bAppliedConditional = true;
							if (bShowDebugLogs)
							{
								UE_LOG(LogTemp, Warning, TEXT("[STATION] Applied Conditional GE %s (Tag: %s) to %s"), 
									*EffectClass->GetName(), *Entry.StateTag.ToString(), *Item->GetName());
								if (GEngine)
								{
									GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("%s: Applied Conditional Effect %s to %s"), *GetName(), *EffectClass->GetName(), *Item->GetName()));
								}
							}
						}
					}
				}
			}
		}
	}


	if (!bAppliedConditional)
	{
		for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectsToApply)
		{
			if (EffectClass)
			{
				FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
				EffectContext.AddInstigator(this, this);
				FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
				if (SpecHandle.IsValid())
				{
					FActiveGameplayEffectHandle Handle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
					if (Handle.IsValid())
					{
						ActiveHandles->Add(Handle);
						if (bShowDebugLogs)
						{
							UE_LOG(LogTemp, Warning, TEXT("[STATION] Applied Base GE %s to %s"), 
								*EffectClass->GetName(), *Item->GetName());
							if (GEngine)
							{
								GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("%s: Applied Base Effect %s to %s"), *GetName(), *EffectClass->GetName(), *Item->GetName()));
							}
						}
					}
				}
			}
		}
	}
}
