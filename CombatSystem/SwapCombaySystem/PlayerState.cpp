
// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState.h"
#include "Net/UnrealNetwork.h"

APlayerState::APlayerState()
{
    bReplicates = true;
    NetUpdateFrequency = 30.f;
}

// void APlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const
// {
//     Super::GetLifetimeReplicatedProps(Out);
//     // DOREPLIFETIME(APlayerState, BaseStats);
//     // DOREPLIFETIME(APlayerState, Damage);
//     // DOREPLIFETIME(APlayerState, Defense);
//     // DOREPLIFETIME(APlayerState, Computed);
//     // DOREPLIFETIME(APlayerState, SelectedItemIndex);
//     // DOREPLIFETIME(APlayerState, SelectedEquipmentIndex);
// }

void APlayerState::RecalculateTotals()
{
    Computed.FinalAttack = Damage.Physical + BaseStats.PhysicalStrength * 2.f;
    Computed.FinalDefense = Defense.Physical + BaseStats.Dexterity * 1.5f;
}

//void APlayerState::OnRep_BaseStats() {}
//void APlayerState::OnRep_Damage() {}
//void APlayerState::OnRep_Defense() {}
//void APlayerState::OnRep_Computed() {}
