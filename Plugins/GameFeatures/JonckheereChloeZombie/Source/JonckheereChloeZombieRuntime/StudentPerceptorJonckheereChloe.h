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
#include "StudentPerceptorJonckheereChloe.generated.h"

class ABaseItem;
class AHouse;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JONCKHEERECHLOEZOMBIERUNTIME_API UStudentPerceptorJonckheereChloe : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UStudentPerceptorJonckheereChloe();
	
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

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
	void GrabItem(ABaseItem* Item);
	bool HasItem(ABaseItem* Item) const;
	void SaveLocation(ABaseItem* Item);
	void SpecifySeenItem(const EItemType& ItemType);
	
	// HOUSE
	void EnterHouse(AHouse* House);
	bool CanVisitHouse(AHouse* House);
	std::vector<AHouse*> m_VisitedHouses{};
	
	// ZOMBIE
	void Attack();
	
	// STEERING
	FVector Seek(const FVector& TargetLocation);
	FVector Flee(const FVector& TargetLocation);
	bool Face(const FVector& TargetLocation, float DeltaT);
};
