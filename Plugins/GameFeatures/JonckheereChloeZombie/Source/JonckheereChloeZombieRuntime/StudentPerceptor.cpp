// Fill out your copyright notice in the Description page of Project Settings.


#include "StudentPerceptor.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "IDetailTreeNode.h"
#include "Items/BaseItem.h"
#include "Survivor/SurvivorPawn.h"

UStudentPerceptor::UStudentPerceptor()
{
	PrimaryComponentTick.bCanEverTick = true;
	m_ItemsInInventory.SetNum(5);
}

void UStudentPerceptor::BeginPlay()
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
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptor::OnPerceptionUpdated);
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

void UStudentPerceptor::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (m_pBlackBoard == nullptr) return;
	if (Stimulus.WasSuccessfullySensed())
	{
		// ITEMS
		//*******
		if (ABaseItem* Item = dynamic_cast<ABaseItem*>(Actor))
		{
			GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, 
	FString::Printf(TEXT("Item: %s"), *ItemEnumToString(Item->GetItemType())));
			if (Item->GetItemType() == EItemType::Garbage) return; // skip garbage
			
			// Grab if not in inv
			
			if (not m_ItemsInInventory.Contains(Item))
			{
				m_pInventory->GrabItem(GetFreeSlot(), Item);
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
	}
	
	
}

FString UStudentPerceptor::ItemEnumToString(const EItemType& itemType) const
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

int UStudentPerceptor::GetFreeSlot() const
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

void UStudentPerceptor::UseItem(int SlotIdx)
{
	m_pInventory->UseItem(SlotIdx);
}

void UStudentPerceptor::RemoveItem(int SlotIdx)
{
	if (m_pInventory->RemoveItem(SlotIdx))
	{
		m_ItemsInInventory.RemoveAt(SlotIdx);
	}
}

void UStudentPerceptor::GrabItem(ABaseItem* Item)
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

void UStudentPerceptor::SaveLocation(ABaseItem* Item)
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

void UStudentPerceptor::SpecifySeenItem(const EItemType& itemType)
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
