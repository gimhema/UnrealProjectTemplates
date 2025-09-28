
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DRSkillComponent.generated.h"

class BaseCharacter;

/** 키 슬롯(Q, W, E, R, T) */
UENUM(BlueprintType)
enum class ESkillSlot : uint8
{
    Q UMETA(DisplayName = "Q"),
    W UMETA(DisplayName = "W"),
    E UMETA(DisplayName = "E"),
    R UMETA(DisplayName = "R"),
    T UMETA(DisplayName = "T"),
};

/** 활성화 결과 */
UENUM(BlueprintType)
enum class ESkillActivateResult : uint8
{
    Success,
    OnCooldown,
    NotEnoughResource,
    InvalidClass,
    NoWorld
};

/** 스폰 파라미터(조준/회전/메타) */
USTRUCT(BlueprintType)
struct FSkillSpawnParams
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FVector Direction = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FRotator OverrideRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 Meta = 0;
};

/** 슬롯별 설정(에디터에서 세팅) */
USTRUCT(BlueprintType)
struct FSkillSlotConfig
{
    GENERATED_BODY()

    /** 스폰될 스킬 액터 클래스 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Setup")
    TSubclassOf<AActor> SkillObjectClass = nullptr;

    /** 스폰 소켓(없으면 루트/액터 위치) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Setup")
    FName SpawnSocketName = NAME_None;

    /** 로컬 오프셋(소켓/루트 기준) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Setup")
    FVector SpawnOffset = FVector::ZeroVector;

    /** 컨트롤 로테이션 사용 여부(원거리/에임) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Setup")
    bool bUseOwnerControlRotation = true;

    /** 스폰 후 오브젝트를 오너에 어태치(오오라/버프 등) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Setup")
    bool bAttachToOwner = false;

    /** 쿨다운(초) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Cost")
    float CooldownSeconds = 0.5f;

    /** 마나/스태미나 코스트 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Cost")
    float ManaCost = 0.f;
};

/** 런타임 상태(마지막 사용 시각 등) */
USTRUCT(BlueprintType)
struct FSkillSlotRuntime
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill|Runtime")
    float LastActivatedTime = -FLT_MAX;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillSpawned, ESkillSlot, Slot, AActor*, SpawnedActor);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DUSKREGION_API SkillComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    SkillComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    /** 오너 캐릭터(자동 캐싱) */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill")
    BaseCharacter* OwnerCharacter = nullptr;

    /** 슬롯 설정들(에디터에서 세팅) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Setup")
    TMap<ESkillSlot, FSkillSlotConfig> SlotConfigs;

    /** 런타임 타이머/상태 */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|Runtime")
    TMap<ESkillSlot, FSkillSlotRuntime> SlotRuntime;

    /** 스폰 이벤트(블루프린트) */
    UPROPERTY(BlueprintAssignable, Category = "Skill|Event")
    FOnSkillSpawned OnSkillSpawned;

public:
    /** 지금 사용 가능? */
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool CanActivate(ESkillSlot Slot) const;

    /** 슬롯 활성화(스폰) */
    UFUNCTION(BlueprintCallable, Category = "Skill")
    ESkillActivateResult ActivateSkill(ESkillSlot Slot, const FSkillSpawnParams& Params, AActor*& OutSpawned);

    /** 임의 트랜스폼으로 스폰(고급) */
    UFUNCTION(BlueprintCallable, Category = "Skill")
    AActor* SpawnSkillAt(ESkillSlot Slot, const FTransform& SpawnTransform, const FSkillSpawnParams& Params);

protected:
    /** 슬롯 설정 가져오기 */
    const FSkillSlotConfig* GetConfig(ESkillSlot Slot) const;

    /** 슬롯 런타임 가져오기(없으면 생성) */
    FSkillSlotRuntime& GetOrCreateRuntime(ESkillSlot Slot);

    /** Transform 생성(소켓/오프셋/컨트롤 회전) */
    FTransform BuildSpawnTransform(ESkillSlot Slot, const FSkillSpawnParams& Params) const;

    /** 스폰 전 트랜스폼 수정 훅 */
    virtual void PreBuildSpawnTransform(ESkillSlot Slot, const FSkillSpawnParams& Params, FTransform& InOutTransform) const;

    /** 스폰 후 액터 설정 훅 */
    virtual void ConfigureSpawnedActor(ESkillSlot Slot, AActor* Spawned, const FSkillSpawnParams& Params);

    /** 리소스 소비 훅(슬롯/코스트 기반) */
    virtual bool ConsumeResource(ESkillSlot Slot, const FSkillSlotConfig& Config);

    /** 현재 월드 타임 */
    float GetWorldTime() const;
};
