// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MeleeHitTracerComponent.h"   // 앞서 만든 본 스윕 트레이서
#include "Curves/CurveFloat.h"
#include "DRAttackComponent.generated.h"


UENUM(BlueprintType)
enum class EAttackPhase : uint8 { Idle, Warmup, Active, Recovery };

USTRUCT(BlueprintType)
struct FMeleeAttackSpec {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    FName Name = TEXT("Light_1");

    // 타임라인 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0"))
    float WarmupTime = 0.12f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0"))
    float ActiveTime = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0"))
    float RecoveryTime = 0.20f;

    // 판정/대미지 (몽타주 없이도 여기에서 제어)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    float Damage = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    float BaseTraceRadius = 8.f;

    // (선택) 스윙 동안 반경을 가변 (0~1 정규화된 진행률 입력)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    UCurveFloat* RadiusScaleCurve = nullptr;

    // 트레이서 미세 설정 덮어쓰기
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    int32 SamplesAlongBlade = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    float MaxStepDistance = 25.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    int32 ExtraSubdivisions = 1;

    // 콤보
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    int32 NextIndex = INDEX_NONE;  // -1이면 콤보 종료

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    float InputBufferOpen = 0.08f; // Active 끝 - N초 전부터 입력을 버퍼링
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttackPhaseChanged, EAttackPhase, Phase, FName, AttackName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackHit, const FHitResult&, Hit);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_NAME_API UMeeleAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMeeleAttackComponent();


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
    UMeleeHitTracerComponent* Tracer = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
    TArray<FMeleeAttackSpec> AttackList;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
    int32 DefaultAttackIndex = 0;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
    bool bServerAuthoritative = true;

    // 입력 처리 API
    UFUNCTION(BlueprintCallable, Category = "Melee")
    void RequestAttack();              // 사용자 입력 (버퍼링 포함)

    UFUNCTION(BlueprintCallable, Category = "Melee")
    bool IsBusy() const { return Phase != EAttackPhase::Idle; }

    // 이벤트(애님BP/이펙트 연동용)
    UPROPERTY(BlueprintAssignable, Category = "Melee")
    FOnAttackPhaseChanged OnPhaseChanged;

    UPROPERTY(BlueprintAssignable, Category = "Melee")
    FOnAttackHit OnHit; // Tracer의 OnHit를 여기서 다시 브로드캐스트

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
private:

    // 진행 상태
    EAttackPhase Phase = EAttackPhase::Idle;
    int32 CurrentIndex = INDEX_NONE;
    float PhaseElapsed = 0.f;

    bool bBufferedNext = false; // 입력 버퍼 플래그

    // 내부 흐름
    void StartAttack(int32 Index);
    void SetPhase(EAttackPhase NewPhase);
    void UpdatePhase(float DeltaTime);

    // 현재 스펙 헬퍼
    const FMeleeAttackSpec* CurSpec() const;
    float GetNormProgress() const; // 0~1, Warmup/Active/Recovery 구간별 상대 시간

    // 트레이서/대미지 설정 동기화
    void ApplySpecToTracer(const FMeleeAttackSpec& Spec);
    void OnTracerHit(const FHitResult& Hit);

};
