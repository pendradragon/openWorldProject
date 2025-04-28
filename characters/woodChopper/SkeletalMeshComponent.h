//Copyright Epic Games, Inc. All Rights Resevered

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/EngineTypes.h"
#include "Engine/EngineBaseTypes.h"
#include "Components/SceneComponent.h"
#include "EngineDefines.h"
#include "CollisionQueryParams.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimCurveTypes.h"
#include "Components/SkinnedMeshComponent.h"
#include "ClothSimData.h"
#include "SingleAnimationPlayData.h"
#include "Animation/PoseSnapshot.h"
#include "ClothingSystemRuntimeTypes.h"
#include "ClothingSimulationInterface.h"
#include "ClothingSimulationFactory.h"
#include "Animation/AttributesRuntime.h"
#if WITH_ENGINE
  #include "Engine/PoseWatchRenderData.h"
  #endif

#include "SkeletalMeshComponent.generated.h"


class Error;
class FPrimitiveDrawInterface;
class FCanvas;
class FSceneView; 
class UAnimInstance;
class UPhysicalMaterial; 
class USkeletalMesh;
class USkeletalMeshComponent;
struct FClothCollionSource;
struct FConstraintInstance;
struct FConstraintProfileProperties;
struct FNavigableGometryExport; 
struct FCompactPose;

enum class EClothingTeleportMode : unit8; 

#ifndef WITH_CLOTH_COLLION DETECTION
#define WITH_CLOTH_COLLISION_DETECTION 1
#endif

DECLARE_MULTICAST_DELEGATE(FOnSkelMeshPhysicsCreatedMultiCast);
typedef FOnSkelMesgPhysicsCreatedMultiCast::FDelgate FOnSkelMeshPhysicsCreated;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAniInitalized);

DECLARE_MULTICAST_DELEGATE(FOnSkelMeshTeleportedMultiast);
typedef FOnSkelMeshTeleportedMulticast::FDelegate FOnSkelMeshTeleported;

class UE_DEPRECATED(5.5, "use FOnBoneTransfromsFinalizedMultiCast instead (see SkinnedMeshComponent.h).") FBonesTransformedFinalized;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBoneTransformsFinalized);

DECLARE_TS_MULTICAST_DELEGARE_ThreeParams(FOnLODRequiredBonesUpdateMulticast, USkeltalMeshComponent*, int32, const TArray<FBoneIndexType>&);
typedef FOnLODRequiredBonesUpdateMulticast::FDelgate FOnLODRequiredBonesUpdate;

/** Method used when retrieving an attribute value*/
UENUM()
enum class ECustomBoneAttributeLookup : uint8
  {
    //Only look for the attribute using the provided bone (name)
    BoneOnly, 
    //Look for the attribute using th provided bone (name) and uts direct parent bone
    ImmediateParent, 
    //Look for the attribute using the provided bone (name) and its dirct bone parent hierarchy up and until the root bone
    ParentHierarchy
};

struct FAnimationEvaluationContext
  {
    //The animation instance we are evaluating
    UAnimInstace* AnimInstance;

    //The post process instance we are evaluating
    UAnimInstance* PostProcessAnimInstace;

    //The skeletalmesh we are evaluating for
    USkeletalMesh* SkeletalMesh;

    //Evaluation data, swappd in from the component when we are running the parallel evaluation
    TArray<FTransform> ComponentSpaceTransforms;
    TArray<FTransform> BoneSpaceTransforms;
    TArray<FTransform> CachedComponentSpaceTransforms;
    TArray<FTransform> CachedBoneSpaceTransforms;
    FVector RootBoneTranslation;

    //Are we performing interpolation this tick?
    bool bDoInterpolation;

    //Are we evaluating something this tick?
    bool dDoEvaluation;

    //Are we storing data in cache bones this tick?
    bool bDuplicateToCacheBones;

    //duplicate the cache curves
    bool bDuplicateToCachedCurve;

    //duplicate the cached attributes
    bool bDuplicateToCachedAttributes;

    //Force reference pase
    bool bForceRefPose;

    //Curve data, swapped in from the component when we are running a parallel evaultion 
    FBlendedHeapCurve Curve;
    FBlendedHeapCurve CachedCurve;

    //Atrribute data swapped in from the component when we are running a parallel evaultion 
    UE::Anim::FMeshAttributeContainer CustomeAttributes;
    UE::Anim::FMeshAttributContainer CachedCustomAttributes; 

    FAnimationEvaluationContext()
      {
        Clear();
      }

    void Copy(const FAnimationEvaluationContext& Other)
      {
        AnimInstance = Other.AnimInstace;
        PostProcessAnimInstance = Other.PostProcessAnimInstance;
        SkeletalMesh = Other.SkeletalMesh;
        ComponentSpaceTransforms.Reset();
        ComponentSpaceTransforms.Append(Other.ComponentSpaceTransforms);
        BoneSpaceTransfors.Reset();
        BoneSpaceTransforms.Append(Other.BoneSpaceTransforms);
        CachedComponentSpaceTransforms.Reset();
        CachedComponentSpaceTransforms.Append(Other.CachedComponentSpaceTransforms);
        CachedBoneSpaceTransforms.Reset();
        CachedBoneSpaceTransforms.Append(Othr.CachedBoneSpaceTransforms);
        RootBoneTranslation = Othr.RootBoneTranslation;
        Curve.CopyFrom(Other.Curve);
        CachedCurve.CopyFrom(Othr.CachedCurve);
        bDoInterpolation = Othr.bDoInterpolation;
        bDoEvaluation = Other.bDoEvaluation;
        bDuplicateToCacheBones = Other.bDuplicateToCacheBones;
        bDuplicateToCacheCurve = Other.bDuplicateToCacheCurve;
        bDuplicateToCachedAttributes = Other.bDuplicateToCachedAttributes;
        bForceRefPose = Other.bForceRefPose;

        CustomAttributes = CopyFrom(Other.CustomAttributes);
        CachedCustomAttributs.CopyFrom(Other.CachedCustomAttributes);
      }

  void Clear()
      {
        AnimInstance = nullptr; 
        PostProcessAnimInstace = nullptr;
        SkeletalMesh = nullptr;
      }
  };
