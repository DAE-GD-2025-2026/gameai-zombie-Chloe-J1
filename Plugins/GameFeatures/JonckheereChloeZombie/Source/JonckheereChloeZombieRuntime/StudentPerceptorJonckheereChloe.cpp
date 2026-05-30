// Fill out your copyright notice in the Description page of Project Settings.


#include "StudentPerceptorJonckheereChloe.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "IDetailTreeNode.h"
#include "Items/BaseItem.h"
#include "Village/House/House.h"
#include "Survivor/SurvivorPawn.h"

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
	GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Blue, 
	FString::Printf(TEXT("Item: %s"), *Actor->GetName()));
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
	// 	if (ABaseItem* Item = dynamic_cast<ABaseItem*>(Actor))
	// 	{
	// 		GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, 
	// FString::Printf(TEXT("Item: %s"), *ItemEnumToString(Item->GetItemType())));
	// 		if (Item->GetItemType() == EItemType::Garbage) return; // skip garbage
	// 		
	// 		// Grab if not in inv
	// 		
	// 		if (not m_ItemsInInventory.Contains(Item))
	// 		{
	// 			GrabItem(Item);
	// 		}
	// 		// Save location
	// 		else
	// 		{
	// 			SaveLocation(Item);
	// 		}
	// 		
	// 		SpecifySeenItem(Item->GetItemType());
	// 		m_pBlackBoard->SetValueAsBool("SawItem", true);
	// 		m_pBlackBoard->SetValueAsVector("ItemLocation", Item->GetActorLocation());
	// 	}
		
		// ZOMBIES
		//********
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
			return index;
		}
	}
	return -1; // inv full
}

void UStudentPerceptorJonckheereChloe::UseItem(int SlotIdx)
{
	m_pInventory->UseItem(SlotIdx);
}

void UStudentPerceptorJonckheereChloe::RemoveItem(int SlotIdx)
{
	if (m_pInventory->RemoveItem(SlotIdx))
	{
		m_ItemsInInventory.RemoveAt(SlotIdx);
	}
}

void UStudentPerceptorJonckheereChloe::GrabItem(ABaseItem* Item)
{
	GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, 
	FString::Printf(TEXT("Item: %d"), GetFreeSlot()));
	GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, 
	FString::Printf(TEXT("GRAB ITEM")));
	if (m_pInventory->GrabItem(GetFreeSlot(), Item))
	{
		m_ItemsInInventory.Add(Item);
	}
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

void UStudentPerceptorJonckheereChloe::SpecifySeenItem(const EItemType& itemType)
{
	switch (itemType)
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

void UStudentPerceptorJonckheereChloe::EnterHouse(AHouse* House)
{
	m_pBlackBoard->SetValueAsBool("SawHouse", true);
	m_pBlackBoard->SetValueAsObject("LastVisitedHouse", House);
	m_pBlackBoard->SetValueAsVector("HouseLocation", House->GetBounds().Origin);
}

bool UStudentPerceptorJonckheereChloe::CanVisitHouse(AHouse* House)
{
	GEngine->AddOnScreenDebugMessage(5, 5.f, FColor::Blue, 
	FString::Printf(TEXT("CALC CAN VISIT")));
	if (House == m_pBlackBoard->GetValueAsObject("LastVisitedHouse"))
	{
		GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, 
	FString::Printf(TEXT("JSUT VISITED")));
		return false;
	}
	
	const float RevisitTime{50.f};
	auto itr = std::find_if(m_VisitedHouses.begin(), m_VisitedHouses.end(), [&House](const HouseInfo& info)
	{
		return info.House == House;
	});
	if (itr == m_VisitedHouses.end())
	{
		HouseInfo HouseInfo{};
		HouseInfo.House = House;
		HouseInfo.VisitedTimestamp = GetWorld()->GetTimeSeconds();
		m_VisitedHouses.push_back(HouseInfo);
		return true;
	}
	if (GetWorld()->GetTimeSeconds() - itr->VisitedTimestamp <= RevisitTime)
	{
		itr->VisitedTimestamp = GetWorld()->GetTimeSeconds();
		return true;	
	}
	GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, 
	FString::Printf(TEXT("cannot enter")));
	return false;
}
