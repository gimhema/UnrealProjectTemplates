
// Fill out your copyright notice in the Description page of Project Settings.


#include "DRSkillComponent.h"
#include "GameFramework/Controller.h"
#include "Components/SkeletalMeshComponent.h"
#include "BaseCharacter.h"
#include "Engine/World.h"

SkillComponent::SkillComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void SkillComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
}

void SkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

const FSkillSlotConfig* SkillComponent::GetConfig(ESkillSlot Slot) const
{
    return SlotConfigs.Find(Slot);
}

FSkillSlotRuntime& SkillComponent::GetOrCreateRuntime(ESkillSlot Slot)
{
    FSkillSlotRuntime* Found = SlotRuntime.Find(Slot);
    if (!Found)
    {
        FSkillSlotRuntime NewRt;
        SlotRuntime.Add(Slot, NewRt);
        Found = SlotRuntime.Find(Slot);
    }
    return *Found;
}

bool SkillComponent::CanActivate(ESkillSlot Slot) const
{
    if (!GetWorld()) return false;

    const FSkillSlotConfig* Cfg = GetConfig(Slot);
    if (!Cfg || !Cfg->SkillObjectClass) return false;

    const FSkillSlotRuntime* Rt = SlotRuntime.Find(Slot);
    const float Now = GetWorldTime();
    const float Last = Rt ? Rt->LastActivatedTime : -FLT_MAX;
    if (Now - Last < Cfg->CooldownSeconds) return false;

    // 리소스 체크는 ConsumeResource에서 수행(여기서는 true로 가정)
    return true;
}

ESkillActivateResult SkillComponent::ActivateSkill(ESkillSlot Slot, const FSkillSpawnParams& Params, AActor*& OutSpawned)
{
    OutSpawned = nullptr;
    if (!GetWorld()) return ESkillActivateResult::NoWorld;

    const FSkillSlotConfig* Cfg = GetConfig(Slot);
    if (!Cfg || !Cfg->SkillObjectClass) return ESkillActivateResult::InvalidClass;

    FSkillSlotRuntime& Rt = GetOrCreateRuntime(Slot);

    const float Now = GetWorldTime();
    if (Now - Rt.LastActivatedTime < Cfg->CooldownSeconds)
        return ESkillActivateResult::OnCooldown;

    if (!ConsumeResource(Slot, *Cfg))
        return ESkillActivateResult::NotEnoughResource;

    FTransform SpawnXf = BuildSpawnTransform(Slot, Params);
    PreBuildSpawnTransform(Slot, Params, SpawnXf);

    OutSpawned = SpawnSkillAt(Slot, SpawnXf, Params);
    if (OutSpawned)
    {
        Rt.LastActivatedTime = Now;

        if (Cfg->bAttachToOwner && OwnerCharacter)
        {
            OutSpawned->AttachToActor(OwnerCharacter, FAttachmentTransformRules::KeepWorldTransform);
        }

        OnSkillSpawned.Broadcast(Slot, OutSpawned);
        return ESkillActivateResult::Success;
    }

    return ESkillActivateResult::InvalidClass;
}

AActor* SkillComponent::SpawnSkillAt(ESkillSlot Slot, const FTransform& SpawnTransform, const FSkillSpawnParams& Params)
{
    const FSkillSlotConfig* Cfg = GetConfig(Slot);
    if (!GetWorld() || !Cfg || !Cfg->SkillObjectClass) return nullptr;

    FActorSpawnParameters SP;
    SP.Owner = OwnerCharacter ? static_cast<AActor*>(OwnerCharacter) : GetOwner();
    SP.Instigator = OwnerCharacter;
    SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AActor* Spawned = GetWorld()->SpawnActor<AActor>(Cfg->SkillObjectClass, SpawnTransform, SP);
    if (Spawned)
    {
        ConfigureSpawnedActor(Slot, Spawned, Params);
    }
    return Spawned;
}

FTransform SkillComponent::BuildSpawnTransform(ESkillSlot Slot, const FSkillSpawnParams& Params) const
{
    const FSkillSlotConfig* Cfg = GetConfig(Slot);
    const AActor* OwnerActor = GetOwner();
    const USkeletalMeshComponent* Mesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;

    FTransform Base;
    if (Cfg && Mesh && Cfg->SpawnSocketName != NAME_None && Mesh->DoesSocketExist(Cfg->SpawnSocketName))
    {
        Base = Mesh->GetSocketTransform(Cfg->SpawnSocketName, RTS_World);
    }
    else
    {
        Base = OwnerActor ? OwnerActor->GetActorTransform() : FTransform::Identity;
    }

    // 오프셋(로컬) 적용
    if (Cfg)
    {
        Base.AddToTranslation(Base.GetRotation().RotateVector(Cfg->SpawnOffset));
    }

    // 회전 결정
    FRotator UseRot = Base.Rotator();
    if (!Params.OverrideRotation.IsNearlyZero())
    {
        UseRot = Params.OverrideRotation;
    }
    else
    {
        FVector Fwd = FVector::ZeroVector;

        if (!Params.Direction.IsNearlyZero())
        {
            Fwd = Params.Direction.GetSafeNormal();
        }
        else if (Cfg && Cfg->bUseOwnerControlRotation && OwnerCharacter && OwnerCharacter->GetController())
        {
            UseRot = OwnerCharacter->GetController()->GetControlRotation();
            Fwd = UseRot.Vector();
        }
        else
        {
            Fwd = Base.GetRotation().Vector();
        }

        if (!Fwd.IsNearlyZero())
        {
            UseRot = Fwd.Rotation();
        }
    }

    Base.SetRotation(UseRot.Quaternion());
    return Base;
}

void SkillComponent::PreBuildSpawnTransform(ESkillSlot /*Slot*/, const FSkillSpawnParams& /*Params*/, FTransform& /*InOutTransform*/) const
{
    // 자식에서 필요 시 오버라이드(지면 스냅/에임 보정 등)
}

void SkillComponent::ConfigureSpawnedActor(ESkillSlot /*Slot*/, AActor* /*Spawned*/, const FSkillSpawnParams& /*Params*/)
{
    // 자식에서 스킬별 초기화(데미지/속도/팀/시전자 전달 등)
}

bool SkillComponent::ConsumeResource(ESkillSlot /*Slot*/, const FSkillSlotConfig& /*Config*/)
{
    // 기본은 자원 소모 없음. 자식에서 OwnerCharacter의 마나/스태미나를 차감하고 부족하면 false 반환
    return true;
}

float SkillComponent::GetWorldTime() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetTimeSeconds() : 0.f;
}
