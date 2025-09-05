// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MeleeHitTracerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMeleeHitDelegate, const FHitResult&, Hit);


UENUM(BlueprintType)
enum class EMeleeTraceShape : uint8
{
    Sphere  UMETA(DisplayName = "Sphere"),
    Box     UMETA(DisplayName = "Box")
};

USTRUCT(BlueprintType)
struct FMeleeTraceConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace")
    EMeleeTraceShape Shape = EMeleeTraceShape::Sphere;

    // Sphere
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace", meta = (EditCondition = "Shape==EMeleeTraceShape::Sphere", ClampMin = "1.0"))
    float Radius = 8.f;

    // Box (Half extents)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace", meta = (EditCondition = "Shape==EMeleeTraceShape::Box"))
    FVector BoxHalfExtents = FVector(6, 3, 3);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel1; // e.g., MeleeTrace

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace", meta = (ClampMin = "1.0"))
    float MaxStepDistance = 25.f; // 프레임 사이 이동이 클 때 분할 스윕

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace", meta = (ClampMin = "1"))
    int32 ExtraSubdivisions = 1; // 추가 샘플 분할(과도한 터널링 방지)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Debug")
    bool bDebugDraw = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Debug", meta = (EditCondition = "bDebugDraw"))
    float DebugLifeTime = 0.2f;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_NAME_API UMeleeHitTracerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMeleeHitTracerComponent();

    // 트레이스를 돌 무기 메시 + 소켓
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Sockets")
    USkeletalMeshComponent* WeaponMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Sockets")
    FName RootSocketName = TEXT("BladeRoot");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Sockets")
    FName TipSocketName = TEXT("BladeTip");

    // 트레이스/대미지 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace")
    FMeleeTraceConfig TraceConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Damage")
    float Damage = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Damage")
    TSubclassOf<UDamageType> DamageTypeClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Damage")
    bool bHitEachActorOncePerWindow = true;

    // 네트워크: 서버에서만 대미지 확정 (권장)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Networking")
    bool bServerAuthoritative = true;

    // 자기 자신/소유자 무시
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace")
    bool bIgnoreOwner = true;

    // 활성화/비활성화
    UFUNCTION(BlueprintCallable, Category = "Melee")
    void StartTrace();

    UFUNCTION(BlueprintCallable, Category = "Melee")
    void StopTrace(bool bClearHitCache = true);

    UFUNCTION(BlueprintCallable, Category = "Melee")
    void SetWeaponMesh(USkeletalMeshComponent* InMesh) { WeaponMesh = InMesh; }

    UFUNCTION(BlueprintCallable, Category = "Melee")
    void SetSockets(FName InRoot, FName InTip) { RootSocketName = InRoot; TipSocketName = InTip; }

    // 히트 발생 델리게이트 (VFX/SFX 용)
    UPROPERTY(BlueprintAssignable, Category = "Melee")
    FMeleeHitDelegate OnHit;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    bool bActive = false;
    bool bHasPrev = false;

    // 이전 프레임의 소켓 월드 좌표
    FVector PrevRoot = FVector::ZeroVector;
    FVector PrevTip = FVector::ZeroVector;

    // 중복 타격 방지
    TSet<TWeakObjectPtr<AActor>> AlreadyHit;

    // 공통 쿼리 파라미터
    void BuildQueryParams(FCollisionQueryParams& OutQP) const;

    void DoFrameSweep();
    void SweepSegment(const FVector& Start, const FVector& End);
    void HandleHit(const FHitResult& Hit, const FVector& SweepDir);

    // 디버그
    void DrawDebugBetween(const FVector& A, const FVector& B, const FColor& Color, float LifeTime) const;

    // 유틸: 현재 소켓 위치 쌍
    bool GetCurrentBladePoints(FVector& OutRoot, FVector& OutTip) const;

    // 적용 주체(컨트롤러/인스티게이터)
    AController* GetInstigatorControllerSafe() const;
		
};
