// Fill out your copyright notice in the Description page of Project Settings.


#include "DRPartyComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "BaseCharacter.h"


PartyCombatComponent::PartyCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

APlayerController* PartyCombatComponent::GetPC() const
{
    return Cast<APlayerController>(GetOwner());
}

void PartyCombatComponent::InitializeParty()
{
    // 미리 스폰(풀링). 모두 숨김 상태로 대기.
    for (int32 i = 0; i < PartySlots.Num(); ++i)
    {
        if (!PartySlots[i].SpawnedPawn.IsValid())
            SpawnIfNeeded(i);

        if (auto* P = PartySlots[i].SpawnedPawn.Get())
        {
            // 풀링 초기화: 숨김/충돌OFF/틱OFF
            P->SetActorHiddenInGame(true);
            P->SetActorEnableCollision(false);
            P->SetActorTickEnabled(false);
        }
    }

    // 시작 캐릭터 선택(없으면 0번)
    if (ActiveIndex == INDEX_NONE && PartySlots.Num() > 0)
    {
        SwapTo(0);
    }
}

ABaseCharacter* PartyCombatComponent::SpawnIfNeeded(int32 SlotIndex)
{
    if (!PartySlots.IsValidIndex(SlotIndex)) return nullptr;
    if (PartySlots[SlotIndex].SpawnedPawn.IsValid())
        return PartySlots[SlotIndex].SpawnedPawn.Get();

//    UClass* Cls = PartySlots[SlotIndex].CharacterClass.LoadSynchronous();
    UClass* Cls = nullptr;
    if (!Cls) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    UWorld* World = GetWorld();
    if (!World) return nullptr;

    // 초기는 월드 원점에 스폰, 실제 전환 시 위치 스냅
    auto* NewC = World->SpawnActor<ABaseCharacter>(Cls, FTransform::Identity, Params);
    PartySlots[SlotIndex].SpawnedPawn = NewC;
    return NewC;
}

void PartyCombatComponent::SwapTo(int32 SlotIndex)
{
    if (!PartySlots.IsValidIndex(SlotIndex)) return;
    if (ActiveIndex == SlotIndex) return;

    APlayerController* PC = GetPC();
    if (!PC) return;

    ABaseCharacter* OldPawn = Cast<ABaseCharacter>(PC->GetPawn());
    ABaseCharacter* NewPawn = SpawnIfNeeded(SlotIndex);
    if (!NewPawn) return;

    // 위치/회전/속도 스냅(기존 폰이 있으면 그 위치로)
    if (OldPawn)
    {
        NewPawn->SetActorTransform(OldPawn->GetActorTransform());
        // 필요시 이동컴포넌트 속도도 스냅 가능
        // if (auto* MC = OldPawn->GetCharacterMovement()) { ... }
    }

    // 먼저 이전 폰 비활성
    if (OldPawn)
        DeactivatePawn(OldPawn);

    // Possess 교체
    PC->Possess(NewPawn);

    // 새 폰 활성화
    ActivatePawn(NewPawn, OldPawn);

    ActiveIndex = SlotIndex;
}

void PartyCombatComponent::ActivatePawn(ABaseCharacter* NewPawn, ABaseCharacter* OldPawn)
{
    if (!NewPawn) return;

    NewPawn->SetActorHiddenInGame(false);
    NewPawn->SetActorEnableCollision(true);
    NewPawn->SetActorTickEnabled(true);

    // 연출(카메라 컷/FX/사운드) 필요시 여기서 처리
    // UGameplayStatics::SpawnEmitterAtLocation(...);

    // PlayerState의 캐시 갱신은 ABaseCharacter::PossessedBy/OnRep_PlayerState에서 자동 처리되도록 구성해둠
}

void PartyCombatComponent::DeactivatePawn(ABaseCharacter* Pawn)
{
    if (!Pawn) return;
    Pawn->SetActorHiddenInGame(true);
    Pawn->SetActorEnableCollision(false);
    Pawn->SetActorTickEnabled(false);
}

ABaseCharacter* PartyCombatComponent::GetActivePawn() const
{
    if (auto* PC = GetPC())
        return Cast<ABaseCharacter>(PC->GetPawn());
    return nullptr;
}
