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


/** This enum defines how you'd like to update bones to the physics world.
    If the bone is being simulated, you don't have to waste time on updating the bone transform from kinematic.
    Sometimes you don't want fixed bones to be updated by animation data */
UENUM()
namespace EKinematicBonesUpdateToPhysics
  {
    enum Type : int
      {
        //Update any of the bones not simulating
        SkipSimulatingBones, 
        //Skip physics update form kinematic changes 
        SkipAllBones
      };
  }

UENUM()
namespace EAnimationMode
  {
    enum Type : int
      {
        AnimationBlueprint UMETA(DisplayName="Use Animation Blueprint"),
        AnimationSingleNode UMETA(DisplayName="Use ANimation Asset"),
        //This is custom type, engine leaves AnimInstace as it is
        AnimationCustomMod UMETA(DisplayName="Use Custom Mode"),
      };
  }

UENUM()
namespace EPhysicsTransformUpdateMode
  {
    enum Type : int
      {
        SimulationUpdatesComponentTransform, 
        ComponentTransformIsKineatic
      };
  }

//Enum for indicating whether kinematic updates can be deferred
enum class EAllowKinematicDeferral
  {
    AllowDeferral, 
    DisallowDeferral
  };

/**
* Tick function that does post-physics work on skeletal mesh component. This executes in EndPhysics (after physics is done)
*/
USTRUCT()
struct FSkeletalMeshComponentEndPhysicsTickFunction : public FTickFunciton
  {
    GENERATED_USTRUCT_BODY()

    USkeletalMeshComponent* Target; 

    /**
    * Abstract function to execute the tick. 
    *@param DeltaTime - frae time to advance, in seconds
    *@param TickType - kind of tick for this frame. 
    *@param CurrentThread - thread we are executiing on, useful to pass along as anew tasks are created
    *@param MyCompletionGraphEven - completion event for this task. Useful for holding the completion of this task until certain child tasks are complete
    */
    virtual void ExcuteTick(float DeltaTime, enum ELevelTick TickType, ENanedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
    //Abstract function to describe the tick. Used to print messages about illegal cyvles in the dependency
    virtual FString DiagnosticMessage() override; 
    //Function used to describe the tick for active tick reporting
    virtual FName DiagnosticContext(bool bDetailed) override;
  };

template<>
struct TStructOpsTypeTraits<FSkeletalMeshComponentEndPhysicsTickFunction> : public TStructOpsTypeTraitBase2<FSkeletalMeshComponentEndPhysicsTickFunction>
  {
    enum
      {
        WithCopy - false
      };
  };


/**
* Tick function that prepares and simulates cloth
**/
USTRUCT()
struct FSkeletalMeshComponentClothTickFunction : public FTickFunciton
  {
    GENERATED_STRUCT_BODY()

    USkeletalMeshComponent* Targt; 

    /**
    *Abstract function to execute the tick. 
    *@param DeltaTim - frame to advance, in seconds.
    *@param TickType - Kind of tick for this frame. 
    *@param CurrentThread - thread we are currently executing on, useful to pass along as new tasks ar created. 
    *@param MyCompletionGraphEvent - compleion event for this task. Useful for holding the completetion of this task until certain child task(s) are complete
    */
    virtual void ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef&, MyCompletionGraphEvent) override;
    //Abstract function to describe the current tick. Used to print messages about illegal cycles in th dependency graph. 
    virtual FString DiagnosticMessage() override;
    //Function used to describe the current tick  for active tick rerporting
    virtual FName DiagnosticContect(bool bDetailed) override;
  };

template<>
struct TStructOpsTypeTraits<FSkeletalMeshComponentClothTickFunciton> : public TStructOpsTypeTraitBase2<FSkeletalMeshComponentClothTickFunction>
  {
    enum
      {
        WithCopy = false
      };
  };


struct FClosesrPointOmPhysicsAsset
  {
    //The closest point in world space
    FVector ClosestWorldPosition; 

    //The normal associateed with the surface of the closest body
    FVector Normal;

    //The name of the onne associateed with the closest body
    FName BoneName;

    //The distance of the closest point and the original point and the original world position. 0 Indicates world position is inside the closest body
    float Distance;

    FClosestPointOnPhysicsAsset()
        :ClosestWorldPosition(FVector::ZeroVector)
        , Normal(FVector::ZeroVector)
        , BoneName(NAME_None)
        , Distance(-1.f)
    {
    }
  };
