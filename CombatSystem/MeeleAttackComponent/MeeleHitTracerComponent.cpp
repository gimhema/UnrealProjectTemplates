// Fill out your copyright notice in the Description page of Project Settings.


#include "MeleeHitTracerComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"


UMeleeHitTracerComponent::UMeleeHitTracerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetComponentTickEnabled(false); // StartTrace()에서만 틱 활성화
}

void UMeleeHitTracerComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UMeleeHitTracerComponent::StartTrace()
{
    if (!WeaponMesh)
    {
        // 자동 추론: 소유 액터의 첫 번째 SkeletalMesh 사용 시도
        if (AActor* Owner = GetOwner())
        {
            TArray<USkeletalMeshComponent*> Skeletals;
            Owner->GetComponents(Skeletals);
            if (Skeletals.Num() > 0)
            {
                WeaponMesh = Skeletals[0];
            }
        }
    }

    bActive = (WeaponMesh != nullptr);
    bHasPrev = false; // 첫 프레임은 시드만
    if (bHitEachActorOncePerWindow) AlreadyHit.Reset();
    SetComponentTickEnabled(bActive);
}

void UMeleeHitTracerComponent::StopTrace(bool bClearHitCache)
{
    bActive = false;
    SetComponentTickEnabled(false);
    bHasPrev = false;
    if (bClearHitCache) AlreadyHit.Reset();
}

void UMeleeHitTracerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (bActive)
    {
        DoFrameSweep();
    }
}

void UMeleeHitTracerComponent::BuildQueryParams(FCollisionQueryParams& OutQP) const
{
    OutQP = FCollisionQueryParams(SCENE_QUERY_STAT(MeleeBladeSweep), /*bTraceComplex*/ false);
    if (bIgnoreOwner)
    {
        if (const AActor* Owner = GetOwner())
        {
            OutQP.AddIgnoredActor(Owner);
            // 소유 캐릭터가 무기를 들고 있으면 그 메쉬도 무시
            if (WeaponMesh) OutQP.AddIgnoredComponent(WeaponMesh);
        }
    }
}

AController* UMeleeHitTracerComponent::GetInstigatorControllerSafe() const
{
    if (AActor* Owner = GetOwner()) // ← const 제거
    {
        if (APawn* Pawn = Cast<APawn>(Owner))
        {
            return Pawn->GetController();
        }
        return Owner->GetInstigatorController();
    }
    return nullptr;
}

bool UMeleeHitTracerComponent::GetCurrentBladePoints(FVector& OutRoot, FVector& OutTip) const
{
    if (!WeaponMesh) return false;
    OutRoot = WeaponMesh->GetSocketLocation(RootSocketName);
    OutTip = WeaponMesh->GetSocketLocation(TipSocketName);
    return true;
}

void UMeleeHitTracerComponent::DoFrameSweep()
{
    FVector CurrRoot, CurrTip;
    if (!GetCurrentBladePoints(CurrRoot, CurrTip)) return;

    if (!bHasPrev)
    {
        PrevRoot = CurrRoot;
        PrevTip = CurrTip;
        bHasPrev = true;
        return; // 다음 프레임부터 실제 스윕
    }

    const int32 SamplesAlongBlade = 3; // 루트/중간/팁 3지점
    for (int32 i = 0; i < SamplesAlongBlade; ++i)
    {
        const float T = (SamplesAlongBlade == 1) ? 0.f : (float)i / (SamplesAlongBlade - 1);
        const FVector PrevPoint = FMath::Lerp(PrevRoot, PrevTip, T);
        const FVector CurrPoint = FMath::Lerp(CurrRoot, CurrTip, T);
        SweepSegment(PrevPoint, CurrPoint);
    }

    PrevRoot = CurrRoot;
    PrevTip = CurrTip;
}

void UMeleeHitTracerComponent::SweepSegment(const FVector& Start, const FVector& End)
{
    UWorld* World = GetWorld();
    if (!World) return;

    const float Dist = FVector::Distance(Start, End);
    const int32 StepsByDistance = FMath::Max(1, FMath::CeilToInt(Dist / FMath::Max(1.f, TraceConfig.MaxStepDistance)));
    const int32 TotalSteps = FMath::Max(1, StepsByDistance * FMath::Max(1, TraceConfig.ExtraSubdivisions));

    for (int32 s = 0; s < TotalSteps; ++s)
    {
        const float T0 = (float)s / (float)TotalSteps;
        const float T1 = (float)(s + 1) / (float)TotalSteps;

        const FVector A = FMath::Lerp(Start, End, T0);
        const FVector B = FMath::Lerp(Start, End, T1);
        const FVector Dir = (B - A);
        const FQuat Rot = FQuat::Identity;

        FCollisionQueryParams QP;
        BuildQueryParams(QP);

        if (TraceConfig.Shape == EMeleeTraceShape::Sphere)
        {
            TArray<FHitResult> Hits;
            const bool bHit = World->SweepMultiByChannel(
                Hits, A, B, Rot, TraceConfig.TraceChannel,
                FCollisionShape::MakeSphere(TraceConfig.Radius), QP);

            if (TraceConfig.bDebugDraw)
            {
                DrawDebugBetween(A, B, bHit ? FColor::Red : FColor::Green, TraceConfig.DebugLifeTime);
                DrawDebugSphere(World, B, TraceConfig.Radius, 12, bHit ? FColor::Red : FColor::Green, false, TraceConfig.DebugLifeTime);
            }

            if (bHit)
            {
                for (const FHitResult& H : Hits)
                {
                    HandleHit(H, Dir.GetSafeNormal());
                }
            }
        }
        else // Box
        {
            TArray<FHitResult> Hits;
            const bool bHit = World->SweepMultiByChannel(
                Hits, A, B, Rot, TraceConfig.TraceChannel,
                FCollisionShape::MakeBox(TraceConfig.BoxHalfExtents), QP);

            if (TraceConfig.bDebugDraw)
            {
                DrawDebugBetween(A, B, bHit ? FColor::Red : FColor::Green, TraceConfig.DebugLifeTime);
                DrawDebugBox(World, B, TraceConfig.BoxHalfExtents, Rot, bHit ? FColor::Red : FColor::Green, false, TraceConfig.DebugLifeTime);
            }

            if (bHit)
            {
                for (const FHitResult& H : Hits)
                {
                    HandleHit(H, Dir.GetSafeNormal());
                }
            }
        }
    }
}

void UMeleeHitTracerComponent::HandleHit(const FHitResult& Hit, const FVector& SweepDir)
{
    AActor* Other = Hit.GetActor();
    if (!Other) return;

    if (bHitEachActorOncePerWindow && AlreadyHit.Contains(Other))
    {
        return;
    }

    // 서버 권위 적용
    const bool bCanApplyDamage = (!bServerAuthoritative) || (GetOwner() && GetOwner()->HasAuthority());
    if (bCanApplyDamage && Damage > 0.f)
    {
        AController* InstigatorCtrl = GetInstigatorControllerSafe();
        UGameplayStatics::ApplyPointDamage(
            Other, Damage, SweepDir.IsNearlyZero() ? (GetOwner() ? GetOwner()->GetActorForwardVector() : FVector::ForwardVector) : SweepDir,
            Hit, InstigatorCtrl, GetOwner(), DamageTypeClass ? *DamageTypeClass : UDamageType::StaticClass());
    }

    AlreadyHit.Add(Other);
    OnHit.Broadcast(Hit);
}

void UMeleeHitTracerComponent::DrawDebugBetween(const FVector& A, const FVector& B, const FColor& Color, float LifeTime) const
{
    if (UWorld* World = GetWorld())
    {
        DrawDebugLine(World, A, B, Color, false, LifeTime, 0, 1.f);
    }
}
