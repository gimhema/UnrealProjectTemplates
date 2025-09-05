// Fill out your copyright notice in the Description page of Project Settings.


#include "MeeleAttackComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"


UMeeleAttackComponent::UMeeleAttackComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetComponentTickEnabled(true);
}

void UMeeleAttackComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!Tracer)
    {
        if (AActor* Owner = GetOwner())
            Tracer = Owner->FindComponentByClass<UMeleeHitTracerComponent>();
    }
    if (Tracer)
    {
        Tracer->OnHit.AddDynamic(this, &UMeeleAttackComponent::OnTracerHit);
        // 트레이서는 기본적으로 비활성. AttackComponent가 창을 열 때만 StartTrace/StopTrace
    }
}

void UMeeleAttackComponent::RequestAttack()
{
    // 1) Idle이면 즉시 시작
    if (Phase == EAttackPhase::Idle)
    {
        StartAttack(DefaultAttackIndex);
        return;
    }

    // 2) Active 구간의 "버퍼 오픈" 시점 이후면 다음 타 예약
    if (const FMeleeAttackSpec* S = CurSpec())
    {
        if (Phase == EAttackPhase::Active)
        {
            // Active가 끝나기 직전 InputBufferOpen 만큼 남았으면 버퍼 허용
            if ((S->ActiveTime - PhaseElapsed) <= FMath::Max(0.f, S->InputBufferOpen))
            {
                bBufferedNext = true;
            }
        }
        else if (Phase == EAttackPhase::Recovery)
        {
            // 취향에 따라 Recovery 중에도 버퍼 허용 가능
            bBufferedNext = true;
        }
    }
}

void UMeeleAttackComponent::StartAttack(int32 Index)
{
    if (!AttackList.IsValidIndex(Index) || !Tracer) return;

    CurrentIndex = Index;
    bBufferedNext = false;

    ApplySpecToTracer(*CurSpec());

    SetPhase(EAttackPhase::Warmup);
}

void UMeeleAttackComponent::SetPhase(EAttackPhase NewPhase)
{
    Phase = NewPhase;
    PhaseElapsed = 0.f;

    // 트레이서 토글
    if (Phase == EAttackPhase::Active)
    {
        // Active 시작 시 창 열기
        Tracer->StartTrace();
    }
    else
    {
        // 그 외 구간/상태 전환 시 창 닫기(중복 안전)
        Tracer->StopTrace(/*bClearHitCache=*/Phase == EAttackPhase::Idle);
    }

    // 이벤트 브로드캐스트 (애님BP에서 이걸 받아 상태머신 전이/플립북 교체 등 처리)
    const FName AttackName = CurSpec() ? CurSpec()->Name : NAME_None;
    OnPhaseChanged.Broadcast(Phase, AttackName);
}

const FMeleeAttackSpec* UMeeleAttackComponent::CurSpec() const
{
    return AttackList.IsValidIndex(CurrentIndex) ? &AttackList[CurrentIndex] : nullptr;
}

void UMeeleAttackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    UpdatePhase(DeltaTime);

    if (const FMeleeAttackSpec* S = CurSpec(); S && Tracer && S->RadiusScaleCurve && Phase == EAttackPhase::Active)
    {
        const float t = GetNormProgress(); // 0~1 (Active 내부)
        const float k = S->RadiusScaleCurve->GetFloatValue(FMath::Clamp(t, 0.f, 1.f));
        Tracer->TraceConfig.Radius = FMath::Max(1.f, S->BaseTraceRadius * (k <= 0.f ? 1.f : k));
    }
}

float UMeeleAttackComponent::GetNormProgress() const
{
    if (const FMeleeAttackSpec* S = CurSpec())
    {
        switch (Phase)
        {
        case EAttackPhase::Warmup:  return S->WarmupTime > 0 ? (PhaseElapsed / S->WarmupTime) : 1.f;
        case EAttackPhase::Active:  return S->ActiveTime > 0 ? (PhaseElapsed / S->ActiveTime) : 1.f;
        case EAttackPhase::Recovery:return S->RecoveryTime > 0 ? (PhaseElapsed / S->RecoveryTime) : 1.f;
        default: break;
        }
    }
    return 0.f;
}

void UMeeleAttackComponent::UpdatePhase(float DeltaTime)
{
    if (Phase == EAttackPhase::Idle) return;

    PhaseElapsed += DeltaTime;

    const FMeleeAttackSpec* S = CurSpec();
    if (!S) { SetPhase(EAttackPhase::Idle); return; }

    switch (Phase)
    {
    case EAttackPhase::Warmup:
        if (PhaseElapsed >= S->WarmupTime)
            SetPhase(EAttackPhase::Active);
        break;

    case EAttackPhase::Active:
        if (PhaseElapsed >= S->ActiveTime)
            SetPhase(EAttackPhase::Recovery);
        break;

    case EAttackPhase::Recovery:
        if (PhaseElapsed >= S->RecoveryTime)
        {
            // 콤보 큐가 있으면 다음 타로 이어가기
            if (bBufferedNext && S->NextIndex != INDEX_NONE)
            {
                StartAttack(S->NextIndex);
            }
            else
            {
                SetPhase(EAttackPhase::Idle);
                CurrentIndex = INDEX_NONE;
            }
        }
        break;

    default: break;
    }
}

void UMeeleAttackComponent::ApplySpecToTracer(const FMeleeAttackSpec& Spec)
{
    if (!Tracer) return;

    // 트레이서의 스윕 세부 옵션 동기화
    Tracer->TraceConfig.Radius = Spec.BaseTraceRadius;
    Tracer->TraceConfig.MaxStepDistance = Spec.MaxStepDistance;
    Tracer->TraceConfig.ExtraSubdivisions = Spec.ExtraSubdivisions;

    // 대미지 동기화
    Tracer->Damage = Spec.Damage;
}

void UMeeleAttackComponent::OnTracerHit(const FHitResult& Hit)
{
    // VFX/SFX/카메라 셰이크 브로드캐스트
    OnHit.Broadcast(Hit);
}
