// Fill out your copyright notice in the Description page of Project Settings.


#include "StudentPerceptorJonckheereChloe.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Items/BaseItem.h"
#include "Village/House/House.h"
#include "DrawDebugHelpers.h"
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

void UStudentPerceptorJonckheereChloe::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (m_pBlackBoard->GetValueAsBool("SawZombie"))
	{
		// Walk backwards whilst trying to shoot a zombie
		UObject* Object{m_pBlackBoard->GetValueAsObject("Zombie")};
		if (ABaseZombie* Zombie = Cast<ABaseZombie>(Object))
		{
			FVector ZombieLocation{Zombie->GetActorLocation()};
			if (Face(ZombieLocation, DeltaTime)) // Only shoot if facing the zombie
			{
				Attack();
			}
			const float Radius{100.f};
			if (FVector::Dist(GetOwner()->GetActorLocation(), ZombieLocation) <= Radius)
			{
				FVector Dir = Flee(ZombieLocation);
				Cast<APawn>(GetOwner())->AddMovementInput(Dir);
			}
		}
		if (Object == nullptr)
		{
			m_pBlackBoard->SetValueAsBool("SawZombie", false);
		}
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
			m_pBlackBoard->SetValueAsObject("Zombie", Zombie);
			FVector Dir = Flee(m_pBlackBoard->GetValueAsVector("ZombieLocation"));
			Cast<APawn>(GetOwner())->AddMovementInput(Dir);
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
			if (m_ItemsInInventory[index]->GetValue() == 0) // Drop gun if it has no bullets left
			{
				m_pInventory->RemoveItem(index);
				return;
			}
			
			m_pInventory->UseItem(index);
			GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Red, 
	FString::Printf(TEXT("SHOOT")));
			FVector Start = GetOwner()->GetActorLocation();
			FVector End = Start + GetOwner()->GetActorForwardVector() * 10000.f;

			DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 0.5f);
		}
	}
}

FVector UStudentPerceptorJonckheereChloe::Seek(const FVector& TargetLocation)
{
	FVector Dir{FVector(TargetLocation).GetSafeNormal() - GetOwner()->GetActorLocation()};
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



	if (abs(DeltaAngle) >= Threshold)
	{
		AngularVelocity =  DeltaAngle * DeltaT * RotSpeed;
	}
	else
	{
		AngularVelocity = 0.f;
		return true;
	}
	
	const float MaxAngularSpeed{200.f};
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
