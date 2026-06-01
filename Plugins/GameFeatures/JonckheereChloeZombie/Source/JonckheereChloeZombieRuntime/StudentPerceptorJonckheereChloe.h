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
#include "BehaviorTree/BTTaskNode.h"
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
	
	// STEERING
	FVector Seek(const FVector& TargetLocation);
	FVector Flee(const FVector& TargetLocation);
	bool Face(const FVector& TargetLocation, float DeltaT);
	void Move(const FVector& Direction);
	
	// ZOMBIE
	void AttackBehavior(const FVector& TargetLocation, float DeltaT);
private:
	UBlackboardComponent* m_pBlackBoard{};
	UInventoryComponent* m_pInventory{};
	UHealthComponent* m_pHealth{};
	UStaminaComponent* m_pStamina{};
	int m_MaxHealth{};
	int m_MaxStamina{};
	// ITEMS
	TArray<ABaseItem*> m_ItemsInInventory{};
	
	FString ItemEnumToString(const EItemType& itemType) const;
	int GetFreeSlot() const;
	void GrabItem(ABaseItem* Item);
	bool HasItem(ABaseItem* Item);
	void SaveObject(ABaseItem* Item);
	void SpecifySeenItem(const EItemType& ItemType);
	bool UseItem(const EItemType& ItemType);
	bool IsMoreValuable(ABaseItem* Item);
	
	// HOUSE
	void EnterHouse(AHouse* House);
	bool CanVisitHouse(AHouse* House);
	std::vector<AHouse*> m_VisitedHouses{};
	
	// ZOMBIE
	void Shoot();
	
	
	// STATS
	void ManageHealth();
	void ManageStamina();
};

// TASKS
UCLASS()
class JONCKHEERECHLOEZOMBIERUNTIME_API UFleeTask final : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UFleeTask();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

UCLASS()
class JONCKHEERECHLOEZOMBIERUNTIME_API UAttackTask final : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UAttackTask();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;	
};

UCLASS()
class JONCKHEERECHLOEZOMBIERUNTIME_API UFetchWeapon final : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UFetchWeapon();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
private:
	EBTNodeResult::Type SetWeaponLocation(const FVector& Location, UBlackboardComponent* Blackboard);
};

UCLASS()
class JONCKHEERECHLOEZOMBIERUNTIME_API UFetchFood final : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UFetchFood();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

UCLASS()
class JONCKHEERECHLOEZOMBIERUNTIME_API UFetchMedkit final : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UFetchMedkit();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};