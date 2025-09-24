
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "DRPlayerState.generated.h"

// === 여기서 USTRUCT들을 먼저 선언 ===
USTRUCT(BlueprintType)
struct FDRBaseStats {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Health = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float PhysicalStrength = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Dexterity = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Intelligence = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Spiritual = 10.f;
};

USTRUCT(BlueprintType)
struct FDRElementalDamage {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Physical = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Magical = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Fire = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Ice = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Wind = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Ground = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Dark = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Holy = 0.f;
};

USTRUCT(BlueprintType)
struct FDRElementalDefense {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Physical = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Magical = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Fire = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Ice = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Wind = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Ground = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Dark = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Holy = 0.f;
};

USTRUCT(BlueprintType)
struct FDRComputedTotals {
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float FinalAttack = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float FinalDefense = 0.f;
};

// === UCLASS는 USTRUCT들 뒤에 ===
UCLASS()
class DUSKREGION_API APlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    APlayerState();

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    FDRBaseStats BaseStats;

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    FDRElementalDamage Damage;

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    FDRElementalDefense Defense;

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    FDRComputedTotals Computed;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    int32 SelectedItemIndex = -1;

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    int32 SelectedEquipmentIndex = -1;

    UFUNCTION(BlueprintCallable, Category = "Stats")
    void RecalculateTotals();

protected:
    //UFUNCTION() void OnRep_BaseStats();
    //UFUNCTION() void OnRep_Damage();
    //UFUNCTION() void OnRep_Defense();
    //UFUNCTION() void OnRep_Computed();

    // virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
};
