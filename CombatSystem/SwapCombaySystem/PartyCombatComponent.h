// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DRPartyComponent.generated.h"

USTRUCT(BlueprintType)
struct FPartySlot
{
	GENERATED_BODY()

	// 사용 캐릭터 클래스(블프에서 세팅)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class ADRBaseCharacter> CharacterClass;

	// 런타임에 스폰된 폰(풀링 대상)
	UPROPERTY(Transient)
	TWeakObjectPtr<class ADRBaseCharacter> SpawnedPawn = nullptr;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DUSKREGION_API PartyCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	PartyCombatComponent();

    // 파티 3인 구성 (에디터에서 클래스만 세팅해도 됨)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
    TArray<FPartySlot> PartySlots; // Size=3 권장

    // 현재 활성 슬롯 인덱스
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Party")
    int32 ActiveIndex = INDEX_NONE;

    // 초기화(맵 로드 시, BeginPlay에서 호출)
    UFUNCTION(BlueprintCallable, Category = "Party")
    void InitializeParty();

    // 슬롯 스왑 (0,1,2)
    UFUNCTION(BlueprintCallable, Category = "Party")
    void SwapTo(int32 SlotIndex);

    // 현재 활성 폰 가져오기
    UFUNCTION(BlueprintPure, Category = "Party")
    class ADRBaseCharacter* GetActivePawn() const;

private:
    class APlayerController* GetPC() const;
    class ADRBaseCharacter* SpawnIfNeeded(int32 SlotIndex);
    void ActivatePawn(ADRBaseCharacter* NewPawn, ADRBaseCharacter* OldPawn);
    void DeactivatePawn(ADRBaseCharacter* Pawn);
		
};
