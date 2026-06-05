// Fill out your copyright notice in the Description page of Project Settings.


#include "StudentPerceptorJonckheereChloe.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Items/BaseItem.h"
#include "Village/House/House.h"
#include "DrawDebugHelpers.h"
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
	m_pHealth = GetOwner()->GetComponentByClass<UHealthComponent>();
	m_pStamina = GetOwner()->GetComponentByClass<UStaminaComponent>();
	
	m_MaxHealth = m_pHealth->GetMaxHealth();
	m_MaxStamina = m_pStamina->GetMaxStamina();
	
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

void UStudentPerceptorJonckheereChloe::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)

{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	m_pBlackBoard->SetValueAsFloat("Health", m_pHealth->GetHealth()); // Always update health
	m_pBlackBoard->SetValueAsFloat("Stamina", m_pStamina->GetCurrentStamina()); // Always update stamina
	
	UpdateDistanceToVillage(GetOwner()->GetActorLocation());
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
			else if (IsMoreValuable(Item))
			{
				GrabItem(Item);
			}
			else
			{
				SaveObject(Item);
			}
			
			SpecifySeenItem(Item->GetItemType());
			m_pBlackBoard->SetValueAsBool("SawItem", true);
			m_pBlackBoard->SetValueAsVector("ItemLocation", Item->GetActorLocation());
		}
		
		// ZOMBIES
		//********
		if (ABaseZombie* Zombie = dynamic_cast<ABaseZombie*>(Actor))
		{
			if (m_pBlackBoard->GetValueAsBool("SawZombie") == false) // Prevent overwriting the previous sawn zombie
			{
				m_pBlackBoard->SetValueAsBool("SawZombie", true);
				m_pBlackBoard->SetValueAsObject("Zombie", Zombie);
				FVector Dir = Flee(m_pBlackBoard->GetValueAsVector("ZombieLocation"));
				Cast<APawn>(GetOwner())->AddMovementInput(Dir);
			}
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
		
		if (Item->GetItemType() == EItemType::Shotgun || Item->GetItemType() == EItemType::Pistol)
		{
			m_pBlackBoard->SetValueAsBool("HasWeapon", true);
		}
		else if (Item->GetItemType() == EItemType::Medkit)
		{
			m_pBlackBoard->SetValueAsBool("HasMedkit", true);
		}
		else if (Item->GetItemType() == EItemType::Food)
		{
			m_pBlackBoard->SetValueAsBool("HasFood", true);
		}
	}
}

bool UStudentPerceptorJonckheereChloe::HasItem(ABaseItem* Item)
{
	m_ItemsInInventory = m_pInventory->GetInventory();
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

void UStudentPerceptorJonckheereChloe::SaveObject(ABaseItem* Item)
{
	switch (Item->GetItemType())
	{
	case EItemType::Food:
		m_pBlackBoard->SetValueAsObject("Food", Item);	
		break;
	case EItemType::Medkit:
		m_pBlackBoard->SetValueAsObject("Medkit", Item);	
		break;
	case EItemType::Shotgun:
		m_pBlackBoard->SetValueAsObject("Shotgun", Item);	
		break;
	case EItemType::Pistol:
		m_pBlackBoard->SetValueAsObject("Pistol", Item);	
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

void UStudentPerceptorJonckheereChloe::EnterHouse(AHouse* House)
{
	m_pBlackBoard->SetValueAsBool("SawHouse", true);
	m_pBlackBoard->SetValueAsObject("LastVisitedHouse", House);
	m_pBlackBoard->SetValueAsVector("HouseLocation", House->GetBounds().Origin);
	m_HouseLocations.push_back(House->GetBounds().Origin);
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

void UStudentPerceptorJonckheereChloe::UpdateDistanceToVillage(const FVector& SelfLocation)
{
	float ClosestDistanceToVillage{FLT_MAX};
	for (const auto& HouseLocation : m_HouseLocations)
	{
		float NewDist{(float)FVector::Dist2D(HouseLocation, SelfLocation)};
		if (NewDist < ClosestDistanceToVillage)
		{
			ClosestDistanceToVillage = NewDist;
		}
	}
	m_pBlackBoard->SetValueAsFloat("DistanceToVillage", ClosestDistanceToVillage);
}

void UStudentPerceptorJonckheereChloe::Shoot()
{
	m_ItemsInInventory = m_pInventory->GetInventory();
	// Look for a weapon to use
	EItemType PreferredWeapon{};
	EItemType SecondChoise{};
	
	UObject* Object = m_pBlackBoard->GetValueAsObject("Zombie");
	if (ABaseZombie* Zombie = Cast<ABaseZombie>(Object))
	{
		const float Radius{300.f};
		if (FVector::Dist2D(Zombie->GetActorLocation(), GetOwner()->GetActorLocation()) < Radius)
		{
			PreferredWeapon = EItemType::Shotgun;
			SecondChoise = EItemType::Pistol;
		}
		else
		{
			PreferredWeapon = EItemType::Pistol;
			SecondChoise = EItemType::Shotgun;
		}
	
		for (int index{0}; index < m_ItemsInInventory.Num(); ++index)
		{
			if (m_ItemsInInventory[index] == nullptr) continue;
		
			if (UseItem(PreferredWeapon))
			{
				GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Red, 
		FString::Printf(TEXT("SHOOT")));
				FVector Start = GetOwner()->GetActorLocation();
				FVector End = Start + GetOwner()->GetActorForwardVector() * 10000.f;
				DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 0.5f);
			}
			else if (UseItem(SecondChoise))
			{
				GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Red, 
		FString::Printf(TEXT("SHOOT")));
				FVector Start = GetOwner()->GetActorLocation();
				FVector End = Start + GetOwner()->GetActorForwardVector() * 10000.f;
				DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 0.5f);
			}
		}
		UpdateHasWeapon();
	}
}

void UStudentPerceptorJonckheereChloe::UpdateHasWeapon()
{
	m_ItemsInInventory = m_pInventory->GetInventory();
	for (const auto& Item : m_ItemsInInventory)
	{
		if (Item == nullptr) continue;
		if (Item->GetItemType() == EItemType::Shotgun || Item->GetItemType() == EItemType::Pistol)
		{
			m_pBlackBoard->SetValueAsBool("HasWeapon", true);
			return;
		}
	}
	m_pBlackBoard->SetValueAsBool("HasWeapon", false);
}

void UStudentPerceptorJonckheereChloe::AttackBehavior(const FVector& TargetLocation, float DeltaT)
{
	UpdateHasWeapon();
	if (not m_pBlackBoard->GetValueAsBool("HasWeapon"))
	{
		return;
	}
	if (Face(TargetLocation, DeltaT)) // Only shoot if facing the zombie
	{
		Shoot();
	}
	const float Radius{300.f};
	// Walk backwards whilst trying to shoot a zombie
	if (FVector::Dist(GetOwner()->GetActorLocation(), TargetLocation) <= Radius)
	{
		FVector Dir = Flee(TargetLocation);
		Move(Dir);
	}
}

FVector UStudentPerceptorJonckheereChloe::Seek(const FVector& TargetLocation)
{
	FVector Dir{(FVector(TargetLocation) - GetOwner()->GetActorLocation()).GetSafeNormal()};
	Dir.Z = 0;
	return Dir;
}

FVector UStudentPerceptorJonckheereChloe::Flee(const FVector& TargetLocation)
{
	return Seek(TargetLocation) *= -1;
}

bool UStudentPerceptorJonckheereChloe::Face(const FVector& TargetLocation, float DeltaT)
{
	constexpr float Threshold{0.1f};
	const float RotSpeed{80.f};
	double AngularVelocity{0.f};

	FVector2D AgentForward(GetOwner()->GetActorForwardVector().X, GetOwner()->GetActorForwardVector().Y);

	// calc delta angle
	FVector2D Destination = FVector2D{TargetLocation - GetOwner()->GetActorLocation()};

	double TargetAngle = FMath::Atan2(Destination.Y, Destination.X);
	double ForwardAngle = FMath::Atan2(AgentForward.Y, AgentForward.X);

	double DeltaAngle = TargetAngle - ForwardAngle;
	DeltaAngle = FMath::UnwindRadians(DeltaAngle); // Get smallest angle

	if (abs(DeltaAngle) >= Threshold)
	{
		AngularVelocity =  DeltaAngle * DeltaT * RotSpeed;
	}
	else
	{
		FRotator ExactRotation = (TargetLocation - GetOwner()->GetActorLocation()).ToOrientationRotator();
		ExactRotation.Pitch = 0;
		ExactRotation.Roll = 0;
		GetOwner()->SetActorRotation(ExactRotation);
		return true;
	}
	
	const float MaxAngularSpeed{400.f};
	float const DeltaYaw = FMath::Clamp(AngularVelocity, -1.0f, 1.0f) * MaxAngularSpeed * DeltaT;
				
	FRotator const CurrentRotation{GetOwner()->GetActorForwardVector().ToOrientationRotator()};
	FRotator const DeltaRotation{0, DeltaYaw, 0};
	FRotator const DesiredRotation{CurrentRotation + DeltaRotation};
				
	// We only ever care about yaw
	if (!FMath::IsNearlyEqual(CurrentRotation.Yaw, DesiredRotation.Yaw))
	{
		GetOwner()->SetActorRotation(DesiredRotation);
	}
	return false;
}

void UStudentPerceptorJonckheereChloe::Move(const FVector& Direction)
{
	if (/*m_pStamina->GetCurrentStamina() > 0*/true) // only move when stamina left
	{
		FCollisionQueryParams CollisionParams{};
		CollisionParams.AddIgnoredActor(GetOwner());
		auto const End = GetOwner()->GetActorLocation() + Direction * 70.0f;
		if (FHitResult HitResult{}; 
			GetWorld()->LineTraceSingleByChannel(HitResult, GetOwner()->GetActorLocation(), End, 
				ECC_Pawn, CollisionParams))
		{
			if (HitResult.bBlockingHit)
			{
				FRotator Rotation{0.f, 30.f,0.f};
				GetOwner()->AddActorLocalRotation(Rotation);
				GEngine->AddOnScreenDebugMessage(16, 1.f, FColor::Magenta, FString::Printf(TEXT("BLOCKED")));
			}
		}
		Cast<APawn>(GetOwner())->AddMovementInput(Direction);
	}
}

bool UStudentPerceptorJonckheereChloe::UseItem(const EItemType& ItemType)
{
	m_ItemsInInventory = m_pInventory->GetInventory();
	// Look if we have the correct item we want to use
	for (int index{0}; index < m_ItemsInInventory.Num(); ++index)
	{
		if (m_ItemsInInventory[index] == nullptr) continue;
		if (m_ItemsInInventory[index]->GetItemType() == ItemType)
		{
			m_pInventory->UseItem(index);
			GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, FString::Printf(TEXT("Used item: %s"), *ItemEnumToString(ItemType)));
			if (m_ItemsInInventory[index]->GetValue() == 0) // Drop item if it has no value left
			{
				m_pInventory->RemoveItem(index);
			}
			return true;
		}
	}
	return false;
}

bool UStudentPerceptorJonckheereChloe::IsMoreValuable(ABaseItem* Item)
{
	m_ItemsInInventory = m_pInventory->GetInventory();

	for (int index{0}; index < m_ItemsInInventory.Num(); ++index)
	{
		if (m_ItemsInInventory[index] == nullptr) continue;
		if (m_ItemsInInventory[index]->GetItemType() == Item->GetItemType() && m_ItemsInInventory[index]->GetValue() < Item->GetValue())
		{
			GEngine->AddOnScreenDebugMessage(6, 3.f, FColor::Green, 
	FString::Printf(TEXT("Dropped %s, prev value %i new value %i"), *ItemEnumToString(Item->GetItemType()), m_ItemsInInventory[index]->GetValue(), Item->GetValue()));
			m_pInventory->RemoveItem(index); // Drop so we can pickup the more valuable version
			return true;
		}
	}
	return false;
}

// TASKS
UFleeTask::UFleeTask()
{
	NodeName = "Flee";
	bNotifyTick = true;
}

EBTNodeResult::Type UFleeTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();

	UObject* Object = Blackboard->GetValueAsObject("Zombie");
	ABaseZombie* Zombie = Cast<ABaseZombie>(Object);
	if (!Zombie) return EBTNodeResult::Failed;
	m_Perceptor = Pawn->GetComponentByClass<UStudentPerceptorJonckheereChloe>();
	return EBTNodeResult::InProgress;
}

void UFleeTask::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();

	UObject* Object = Blackboard->GetValueAsObject("Zombie");
	ABaseZombie* Zombie = Cast<ABaseZombie>(Object);
	
	FVector Dir{m_Perceptor->Flee(Zombie->GetActorLocation())};
	m_Perceptor->Move(Dir);
	FVector AwayFromTarget = Pawn->GetActorLocation() + Dir;
	m_Perceptor->Face(AwayFromTarget, DeltaSeconds);
	
	// Distance
	const float SafeDistance{1500.f};
	if (FVector::Dist(Pawn->GetActorLocation(), Zombie->GetActorLocation()) >= SafeDistance)
	{
		GEngine->AddOnScreenDebugMessage(16, 3.f, FColor::Magenta, 
	FString::Printf(TEXT("SAFE SPOT")));
		Blackboard->SetValueAsBool("SawZombie", false);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type USprint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	Cast<ASurvivorPawn>(Pawn)->StartRunning();
	return EBTNodeResult::Succeeded;
}

EBTNodeResult::Type UStopSprint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	Cast<ASurvivorPawn>(Pawn)->StopRunning();
	return EBTNodeResult::Succeeded;
}

UAttackTask::UAttackTask()
{
	NodeName = "Attack";
	bNotifyTick = true;
}

EBTNodeResult::Type UAttackTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::InProgress;
}

void UAttackTask::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	
	UStudentPerceptorJonckheereChloe* Perceptor = Pawn->GetComponentByClass<UStudentPerceptorJonckheereChloe>();
	UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();

	UObject* Object = Blackboard->GetValueAsObject("Zombie");
	ABaseZombie* Zombie = Cast<ABaseZombie>(Object);
	if (Zombie)
	{
		Perceptor->AttackBehavior(Zombie->GetActorLocation(), DeltaSeconds);
	}
	
	if (not IsValid(Zombie))
	{
		GEngine->AddOnScreenDebugMessage(16, 3.f, FColor::Magenta, 
	FString::Printf(TEXT("ZOMBIE DIED")));
		Blackboard->SetValueAsBool("SawZombie", false);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

UFetchWeapon::UFetchWeapon()
{
	NodeName = "Fetch weapon";
}

EBTNodeResult::Type UFetchWeapon::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	
	UStudentPerceptorJonckheereChloe* Perceptor = Pawn->GetComponentByClass<UStudentPerceptorJonckheereChloe>();
	UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
	
	UObject* Object = Blackboard->GetValueAsObject("Shotgun");
	if (ABaseItem* Shotgun = Cast<ABaseItem>(Object))
	{
		FVector Location{Shotgun->GetActorLocation()};
		SetWeaponLocation(Location, Blackboard);
		return EBTNodeResult::Succeeded;
	}
	else
	{
		Object = Blackboard->GetValueAsObject("Pistol");
		if (ABaseItem* Pistol = Cast<ABaseItem>(Object))
		{
			FVector Location{Pistol->GetActorLocation()};
			SetWeaponLocation(Location, Blackboard);
			return EBTNodeResult::Succeeded;
		}
	}
	GEngine->AddOnScreenDebugMessage(7, 1.f, FColor::Orange, FString::Printf(TEXT("No weapons seen")));
	return EBTNodeResult::Failed;
}

void UFetchWeapon::SetWeaponLocation(const FVector& Location, UBlackboardComponent* Blackboard)
{
	Blackboard->SetValueAsVector("WeaponLocation", Location);
	GEngine->AddOnScreenDebugMessage(7, 1.f, FColor::Green, FString::Printf(TEXT("Fetching pistol")));
}

UFetchFood::UFetchFood()
{
	NodeName = "Fetch food";
}

EBTNodeResult::Type UFetchFood::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	
	UStudentPerceptorJonckheereChloe* Perceptor = Pawn->GetComponentByClass<UStudentPerceptorJonckheereChloe>();
	UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
	
	UObject* Object = Blackboard->GetValueAsObject("Food");
	if (ABaseItem* Food = Cast<ABaseItem>(Object))
	{
		FVector Location{Food->GetActorLocation()};
		Blackboard->SetValueAsVector("FoodLocation", Location);
		GEngine->AddOnScreenDebugMessage(7, 1.f, FColor::Green, FString::Printf(TEXT("Fetching food")));
		return EBTNodeResult::Succeeded;
	}

	GEngine->AddOnScreenDebugMessage(7, 1.f, FColor::Orange, FString::Printf(TEXT("No food seen")));
	return EBTNodeResult::Failed;
}

UFetchMedkit::UFetchMedkit()
{
	NodeName = "Fetch medkit";
}

EBTNodeResult::Type UFetchMedkit::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	
	UStudentPerceptorJonckheereChloe* Perceptor = Pawn->GetComponentByClass<UStudentPerceptorJonckheereChloe>();
	UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
	
	UObject* Object = Blackboard->GetValueAsObject("Medkit");
	if (ABaseItem* Food = Cast<ABaseItem>(Object))
	{
		FVector Location{Food->GetActorLocation()};
		Blackboard->SetValueAsVector("MedkitLocation", Location);
		GEngine->AddOnScreenDebugMessage(7, 1.f, FColor::Green, FString::Printf(TEXT("Fetching medkit")));
		return EBTNodeResult::Succeeded;
	}

	GEngine->AddOnScreenDebugMessage(7, 1.f, FColor::Orange, FString::Printf(TEXT("No medkit seen")));
	return EBTNodeResult::Failed;
}

UConsumeMedkit::UConsumeMedkit()
{
	NodeName = "Consume medkit";
}

EBTNodeResult::Type UConsumeMedkit::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	UStudentPerceptorJonckheereChloe* Perceptor = Pawn->GetComponentByClass<UStudentPerceptorJonckheereChloe>();
	if (Perceptor->UseItem(EItemType::Medkit))
		Controller->GetBlackboardComponent()->SetValueAsBool("HasMedkit", false);
	return EBTNodeResult::Succeeded;
}

UConsumeFood::UConsumeFood()
{
	NodeName = "Consume food";
}

EBTNodeResult::Type UConsumeFood::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	UStudentPerceptorJonckheereChloe* Perceptor = Pawn->GetComponentByClass<UStudentPerceptorJonckheereChloe>();
	if (Perceptor->UseItem(EItemType::Food))
		Controller->GetBlackboardComponent()->SetValueAsBool("HasFood", false);
	return EBTNodeResult::Succeeded;
}

