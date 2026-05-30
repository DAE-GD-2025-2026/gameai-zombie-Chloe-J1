// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "Items/ItemType.h"
#include "Common/InventoryComponent.h"
#include "Common/HealthComponent.h"
#include "Common/StaminaComponent.h"
#include <vector>
#include "StudentPerceptor.generated.h"

class ABaseItem;
class AHouse;

struct HouseInfo
{
	float VisitedTimestamp{};
	AHouse* House{};
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JONCKHEERECHLOEZOMBIERUNTIME_API UStudentPerceptor : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UStudentPerceptor();
	
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
private:
	UBlackboardComponent* m_pBlackBoard{};
	UInventoryComponent* m_pInventory{};
	UHealthComponent* m_pHealth{};
	UStaminaComponent* m_pStamina{};
	// ITEMS
	TArray<ABaseItem*> m_ItemsInInventory{};
	
	FString ItemEnumToString(const EItemType& itemType) const;
	int GetFreeSlot() const;
	void UseItem(int SlotIdx);
	void RemoveItem(int SlotIdx);
	void GrabItem(ABaseItem* Item);
	void SaveLocation(ABaseItem* Item);
	void SpecifySeenItem(const EItemType& itemType);
	
	// HOUSE
	std::vector<FVector> m_HouseCorners{};
	int m_CornerIdx{0};
	
	void EnterHouse(AHouse* House);
	bool CanVisitHouse(AHouse* House);
	std::vector<HouseInfo> m_VisitedHouses{};
	
};
