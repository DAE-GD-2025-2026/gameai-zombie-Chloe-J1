// Fill out your copyright notice in the Description page of Project Settings.


#include "StudentPerceptorJonckheereChloe.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "IDetailTreeNode.h"
#include "Items/BaseItem.h"
#include "Village/House/House.h"
#include "Survivor/SurvivorPawn.h"
#include "Zombies/BaseZombie.h"

UStudentPerceptorJonckheereChloe::UStudentPerceptorJonckheereChloe()
{
	PrimaryComponentTick.bCanEverTick = true;
	m_ItemsInInventory.SetNum(5);
}

void UStudentPerceptorJonckheereChloe::BeginPlay()
{
	Super::BeginPlay();

	m_pInventory = GetOwner()->GetComponentByClass<UInventoryComponent>();
	if (m_pInventory == nullptr) UE_LOG(LogTemp, Warning, TEXT("No inventory found!"));
	m_pHealth = GetOwner()->GetComponentByClass<UHealthComponent>();
	if (m_pHealth == nullptr) UE_LOG(LogTemp, Warning, TEXT("No health found!"));
	m_pStamina = GetOwner()->GetComponentByClass<UStaminaComponent>();
	if (m_pStamina == nullptr) UE_LOG(LogTemp, Warning, TEXT("No stamina found!"));
	
	if (auto PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptorJonckheereChloe::OnPerceptionUpdated);
	}
	
	if (APawn* pPawn = Cast<APawn>(GetOwner()))
	{
		if (AAIController* pAIController = Cast<AAIController>(pPawn->GetController()))
		{
			m_pBlackBoard = pAIController->GetBlackboardComponent();
		}
	}
	
	if (m_pBlackBoard != nullptr)
	{
		m_pBlackBoard->SetValueAsBool("SawSomething", false);
	}
}

void UStudentPerceptorJonckheereChloe::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (m_pBlackBoard == nullptr) return;
	if (Stimulus.WasSuccessfullySensed())
	{
		// HOUSE
		//*******
		if (AHouse* House = dynamic_cast<AHouse*>(Actor))
		{
			GEngine->AddOnScreenDebugMessage(5, 5.f, FColor::Blue, 
	FString::Printf(TEXT("SAW HOUSE")));
			if (CanVisitHouse(House))
			{
				EnterHouse(House);
			}
		}
		// ITEMS
		//*******
		if (ABaseItem* Item = dynamic_cast<ABaseItem*>(Actor))
		{
			GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, 
	FString::Printf(TEXT("Item: %s"), *ItemEnumToString(Item->GetItemType())));
			if (Item->GetItemType() == EItemType::Garbage) return; // skip garbage
			
			// Grab if not in inv
			
			if (not HasItem(Item))
			{
				GrabItem(Item);
			}
			// Save location
			else
			{
				SaveLocation(Item);
			}
			
			SpecifySeenItem(Item->GetItemType());
			m_pBlackBoard->SetValueAsBool("SawItem", true);
			m_pBlackBoard->SetValueAsVector("ItemLocation", Item->GetActorLocation());
		}
		
		// ZOMBIES
		//********
		if (ABaseZombie* Zombie = dynamic_cast<ABaseZombie*>(Actor))
		{
			m_pBlackBoard->SetValueAsBool("SawZombie", true);
			Attack();
		}
	}
}

FString UStudentPerceptorJonckheereChloe::ItemEnumToString(const EItemType& itemType) const
{
	switch (itemType)
	{
	case EItemType::Garbage:
		return "Garbage";
	case EItemType::Food:
		return "Food";
	case EItemType::Medkit:
		return "Medkit";
	case EItemType::Shotgun:
		return "Shotgun";
	case EItemType::Pistol:
		return "Pistol";
	}
	return "Unknown";
}

int UStudentPerceptorJonckheereChloe::GetFreeSlot() const
{
	for (int index = 0; index < m_ItemsInInventory.Num(); ++index)
	{
		if (m_ItemsInInventory[index] == nullptr)
		{
			GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, 
	FString::Printf(TEXT("Free slot: %i"), index));
			return index;
		}
	}
	return m_ItemsInInventory.Num(); // inv full
}

void UStudentPerceptorJonckheereChloe::GrabItem(ABaseItem* Item)
{
	m_ItemsInInventory = m_pInventory->GetInventory();
	
	int FreeIdx{GetFreeSlot()};
	if (m_pInventory->GrabItem(FreeIdx, Item))
	{
		GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, 
	FString::Printf(TEXT("GRAB %s"), *Item->GetName()));
		PrintInventory();
	}
}

bool UStudentPerceptorJonckheereChloe::HasItem(ABaseItem* Item) const
{
	for (const auto& ItemInv : m_ItemsInInventory)
	{
		if (ItemInv == nullptr) continue;
		if (Item->GetItemType() == ItemInv->GetItemType())
		{
			return true;
		}
	}
	return false;
}

void UStudentPerceptorJonckheereChloe::SaveLocation(ABaseItem* Item)
{
	switch (Item->GetItemType())
	{
	case EItemType::Food:
		m_pBlackBoard->SetValueAsVector("FoodLocation", Item->GetActorLocation());	
		break;
	case EItemType::Medkit:
		m_pBlackBoard->SetValueAsVector("MedkitLocation", Item->GetActorLocation());	
		break;
	}
}

void UStudentPerceptorJonckheereChloe::SpecifySeenItem(const EItemType& ItemType)
{
	switch (ItemType)
	{
	case EItemType::Food:
		m_pBlackBoard->SetValueAsBool("SawFood", true);
		break;
	case EItemType::Medkit:
		m_pBlackBoard->SetValueAsBool("SawMedkit", true);
		break;
	case EItemType::Shotgun:
		m_pBlackBoard->SetValueAsBool("SawShotgun", true);
		break;
	case EItemType::Pistol:
		m_pBlackBoard->SetValueAsBool("SawPistol", true);
		break;
	}
}

void UStudentPerceptorJonckheereChloe::PrintInventory()
{
	for (const auto& item : m_ItemsInInventory)
	{
		FString Text = TEXT("Inventory:\n");
		for (int i = 0; i < m_ItemsInInventory.Num(); ++i)
		{
			if (m_ItemsInInventory[i] == nullptr)
			{
				Text += FString::Printf(TEXT("Slot %d: Empty\n"), i);
			}
			else
			{
				Text += FString::Printf(TEXT("Slot %d: %s\n"), i, *ItemEnumToString(m_ItemsInInventory[i]->GetItemType()));
			}
		}
		GEngine->AddOnScreenDebugMessage(10, 10000.f, FColor::Yellow, Text);
	}
}

void UStudentPerceptorJonckheereChloe::EnterHouse(AHouse* House)
{
	m_pBlackBoard->SetValueAsBool("SawHouse", true);
	m_pBlackBoard->SetValueAsObject("LastVisitedHouse", House);
	m_pBlackBoard->SetValueAsVector("HouseLocation", House->GetBounds().Origin);
}

bool UStudentPerceptorJonckheereChloe::CanVisitHouse(AHouse* House)
{
	auto itr = std::ranges::find(m_VisitedHouses, House);
	if (itr == m_VisitedHouses.end())
	{
		m_VisitedHouses.push_back(House);
		return true;
	}
	GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Blue, 
	FString::Printf(TEXT("cannot enter")));
	return false;
}

void UStudentPerceptorJonckheereChloe::Attack()
{
	m_ItemsInInventory = m_pInventory->GetInventory();
	// Look for a weapon to use - later we can base the chosen weapon on our health or smth
	for (int index{0}; index < m_ItemsInInventory.Num(); ++index)
	{
		if (m_ItemsInInventory[index] == nullptr) continue;
		if (m_ItemsInInventory[index]->GetItemType() == EItemType::Shotgun || m_ItemsInInventory[index]->GetItemType() == EItemType::Pistol)
		{
			m_pInventory->UseItem(index);
			GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Red, 
	FString::Printf(TEXT("SHOOT")));
		}
	}
}
