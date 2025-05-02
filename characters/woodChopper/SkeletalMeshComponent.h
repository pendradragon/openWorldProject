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


struct FClosesrPointOnPhysicsAsset
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

/**
*SkeletalMeshComponent is usd to create an instance of an animated SkeletalMesh asset.
  *
  *@see https://docs.unrealngine.com/latest/INT/Engine/Content/Types/SkeletalMeshes/
  *@see USkeletalMesh
*/
UCLASS(Blueprintable, ClassGroup=(Rendering, Common), hidecategories=(Object, "Mesh|SkeltalAsset"), config=Engine, editinlinenew, meta=(BlueprintSpawnableComponent), MinimalAPI)
class USkeletalMeshComponent : public USkinnedMeshComponent, public IInterface_CollisionDataProvider
  {
    GENERATED_UCLASS_BODY()

    friend class FSkinnedMeshComponentRecreateRenderStateContext;
    friend class FParallelAnimationCompletionTask;
    friend class USkeletalMesh;
    friend class UAnimInstance;
    friend struct FAnimNode_LinkedAnimGraph;
    friend struct FAnimNode_LinkedAnimLayer;
    friend struct FLinkedInstanceAdapater;
    friend struct FLinkedAnimLayerClassData; 
    friend struct FRigUnit_AnimNextWriteSkeletalMeshComponentPose;

    #if WITH_EDITORONLY_DATA
      private: 
        //The skeletal mesh used by this component
        UE_DEPRECATED(5.1, "This property isn't deprecatd, but a getter and setter must be used at all times to preserve correct operations.")
        UPROPERTY(EditAnywhere, Transient, Setter = SetSkeletalMeshAsset, BlueprintSetter = SetSkeletalMeshAsset, Getter = GetSkeletalMeshAsset, BlueprintGetter = GetSkeletalMeshAsset, Category = Mesh)
        TObjectPtr<USkeletalMesh> SkeletalMeshAsst;
    #endif

    public: 
      /**
      *Set the SkeletalMesh rendered for this mesh.
      */
      UFUNCTION(BlueprintCallable, Category = "Components|SkeeletalMesh")
      void SetSkeletalMeshAsset(USkeletalkMesh* NewMesh) {SetSkeletalMesh(NewMesh, false);}

      /**
      *Get the SkeletalMesh rendered for this mesh
      */
      UFUNCTION(BlueprintPure, Category = "Components|SkeletalMesh")
      ENGINE_API USkeletalMesh* GetSkeletalMeshAsset() const;

      //Gets the shared bone container used between all owned anim instances. Creates it on the first call
      ENGINE_API TSharedPtr<struct FBoneContainer> GetSharedRequiredBones();

      #if WITH_EDITORONLY_DATA
        //The blueprint for creating AnimationScript
        UPROPERTY()
        TObjectPrt<class UAnimBlueprint> AnimationBlueprint_DEPRECATED;
      #endif

      UE_DEPRECATED(4.11, "This property is deprecated. Please AnimClass instead")
      UPROPERTY(BlueprintReadOnly, Category = Animation, meta = (DeprcationMessage = "This property is deprecateed. Please use AnimClass instead."))
      TObjectPtr<class UAnimBlueprintGeneratedClass> AnimBlueprintGeneratedClass;

      //The AnimBlueprint class to use. USe 'SetAnimInstanceClass' to change at runtime
      UPROPERTY(EditAnywhere, BlueprintReadOnly, Settr = SetAnimInstanceClass_Internal, Category = Animation, meta=(EditCondition = bEnableAnimation))
      class TSubclassOf<UAnimInstance> AnimClass; 

      //The active animation graph program instance
      UPROPERTY(transient, NonTransactional)
      TObjectPtr<UAnimInstance> AnimScriptInstance;

      #if WITH_EDITORONLY_DATA
        //Any running anim instances
        UE_DEPRECATD(4.24, "Direct access to this property is deprecated and the array is no longer used. Storage is now in LinkedInstances. Please use GetLinkedAnimInstances() instead.")
        UPROPERTY(trasient)
        TArray<TObjectPtr<UAnimInstance>> SubInstances;
      #endif

      //Post-processing AnimBP to us for th givn skeletal mesh component, overriding the one set in the skeletal mesh asset
      UPROPERTY(transient)
      TSubclassOf<UAnimInstance> OverridePostProcessAnimBP;

      /**
      *Get the post-processing AnimBP to be used for this skeletal mesh component
      *In case an override post-processing AnimBP is set, it will always return the AnimBP class of that
      *Otherwise the one set in skeletal mesh asst will be returned.
      */
      ENGINE_API TSubClassOf<UAnimInstance> GetPostProcessAnimBPClassToBeUsed() const;

      /* An instance created from th PostPhysicsBlueprint property of the skeletal mesh we're using, 
        *Runs after (and receives pose from) th main anim instance. 
      */
      UPROPERTY(transient)
      TObjectPtr<UAnimInstance> PostProcessAnimInstance;

      public:
        /*
        *Set the post-processing AnimBP to being used for this skeletal mesh component
        *In case an override post-processing AnimBP is set, the one set in skeletal mesh asset will be ignored and not used.
        *@param ReinitAnimInstacs can be false when called e.g. from the contruction script in a Blueprint
        *                          the game is running and the anim instances need to be re-initalized
        */
        UFUNCTION(Blueprint Callable, Category = "Components|SkeletalMesh")
        ENGINE_API void SetOverridePostProcessAnimBP(TSubclassOf<UAnimInstance> InPostProcessAnimBlueprint, bool ReinitAnimInstance = true);

        //Toggles whether th post process blueprint will run for this component
        UFUNCTION(BlueprintCallable, Category = "Comoponents|SkeletalMesh")
        ENGINE_API void ToggleDisablePostProcessBlueprint();

        //Get whether the post process blueprint is currently disabled for this component
        UFUNCTION(BlueprintGetter)
        ENGINE_API bool GetDisablePostProcessBlueprint() const;

        /*
        * If it is not currently running, and is st to run, the instance will be reinitalized
        */
        UFUNCTION(BlueprintSetter)
        ENGINE_API void SetDisablePostProcessBlueprint(bool bINDisablePostProcess);

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation, meta = (ShowOnlyInnerProperties, EditCondition = bEnableANimation))
        struct FSingleAnimationPlayData AnimationData;

        //this is an explict copy because this buffer is resued during evaluation
        //we want to have reference an emptied during evaluation
        ENGINE_API TArray<FTransform> GetBoneSpaceTransforms();

        //Get th bone space transforms as an array view
        ENGINE_API TArrayView<const FTransform> GetBoneSpaceTransformsView();

        /*
        *Temporary array of local-space (relative to parent bone) rotation/translation for each bone.
        *This property is not saf to accss during evaluation, so we created wrapper
        */
        UE_DEPRECEATED(4.23, "Direct access to this property is deprecated, please use GetBoneSpaceTransforms instead. We will move to private in th future.")
        TArray<FTransform> BoneSpaceTransforms;

      public: 
        //offset of the root bone from the reference pose. Used to offset bounding box
        UPROPERTY(transient)
        FVector RootBoneTranslation; 

        //If bEnableLinCheckWithBounds is true, the scale of the bounds by this valu before doing line check
        UPROPERTY()
        FVector LineCheckBoundsScale;

        //Temporary storage for curves
        FBlendedHeapCurve AnimCurves;

        //Access cached component space transforms
        ENGINE_API const TArray<FTransform>& GetCachedComponentSpaceTransforms() const;

    private:
      //Any running linked anim instances
      UPROPERTY(transient)
      TArray<TObjectPtr<UAnimInstance>> LinkedInstances;

      //Shared bone container betwee all anim instances owned by this skeletal mesh component
      TSharedPtr<struct FBoneContainer> SharedRequiredBones;

      //Update Rate

      //Cached BoneSpaceTransforms for Update Rate optimization
      UPROPERTY(Transiet)
      TArray<FTransform> CachedBoneSpaceTransforms;

      //Cached SpaceBases for Update Race optimization
      UPROPERTY(Transient)
      TArray<FTransform> CachedComponentSpaceTransforms;

      //Cached Curve result for Update Rate optimization 
      FBlendedHeapCurve CachedCurve;

      //Current and cached atrubute evaluation data, used for Update Rate optimization
      UE::Anim::FMeshAttributeContainer CachedAttributes;
      UE::Anim::FMeshAttributeContainer CustomAtributes;
    public:
      /*
      *Get float type attribute value

      *@param BoneName Name of the bone to retrieve try and retrieve the attribute from
      *@param AttributeName Nam eof the attribute to retrieve
      *@param OutValue (reference) Retrieved atrribute value if found othrwise is set to DefaultValue
      *@param LookupType Dtermines how th attribute is from the specified BoneName (se CUtsomBoneAttributeLookup)
      *@return Whether or not the attribute was successfully retrieved
      */
      UFUNCTION(BlueprintCallable, Category = Category=CustomAttributes)
      ENGINE_API bool GetFloatAttribute_Ref(const FName& BoneName, const FName& AttributeNAme, UPARAM(ref) float& OutValue, ECustomBonAttributeLookup LookupType = ECustomBoneAttribute::BoneOnly);

      /*
      *Get FTransform type attributee value.

      *@param BoneName Name of the bone to retrieve try and retrieve th attribute from 
	    *@param AttributeName Name of the attribute to retrieve
      *@param OutValue (reference) Retrieved attribute value if found, otherwise is set to DefaultValue
      *@param LookupType Determines how the attribute is retrieved from the specified BoneName (see ECustomBoneAttributeLookup)
      *@return Whether or not the atttribute was successfully retrieved
      */
    	UFUNCTION(BlueprintCallable, Category = CustomAttributes)
	ENGINE_API bool GetTransformAttribute_Ref(const FName& BoneName, const FName& AttributeName, UPARAM(ref) FTransform& OutValue, ECustomBoneAttributeLookup LookupType = ECustomBoneAttributeLookup::BoneOnly);
      
      /** 
      * Get integer type attribute value.
      
       * @param BoneName Name of the bone to retrieve try and retrieve the attribute from
       * @param AttributeName Name of the attribute to retrieve
       * @param OutValue (reference) Retrieved attribute value if found, otherwise is set to DefaultValue
       * @param LookupType Determines how the attribute is retrieved from the specified BoneName (see ECustomBoneAttributeLookup)
       * @return Whether or not the atttribute was successfully retrieved
        */
      UFUNCTION(BlueprintCallable, Category = CustomAttributes)
      ENGINE_API bool GetIntegerAttribute_Ref(const FName& BoneName, const FName& AttributeName, UPARAM(ref) int32& OutValue, ECustomBoneAttributeLookup LookupType = ECustomBoneAttributeLookup::BoneOnly);
        
          /** 
         * Get string type attribute value.
        
         * @param BoneName Name of the bone to retrieve try and retrieve the attribute from
         * @param AttributeName Name of the attribute to retrieve
         * @param OutValue (reference) Retrieved attribute value if found, otherwise is set to DefaultValue
         * @param LookupType Determines how the attribute is retrieved from the specified BoneName (see ECustomBoneAttributeLookup)
         * @return Whether or not the atttribute was successfully retrieved
        */
      UFUNCTION(BlueprintCallable, Category = CustomAttributes)
      ENGINE_API bool GetStringAttribute_Ref(const FName& BoneName, const FName& AttributeName, UPARAM(ref) FString& OutValue, ECustomBoneAttributeLookup LookupType = ECustomBoneAttributeLookup::BoneOnly);
     
      /** 
       * Get float type attribute value.
      
       * @param BoneName Name of the bone to retrieve try and retrieve the attribute from
       * @param AttributeName Name of the attribute to retrieve
       * @param DefaultValue In case the attribute could not be found
       * @param OutValue Retrieved attribute value if found, otherwise is set to DefaultValue
       * @param LookupType Determines how the attribute is retrieved from the specified BoneName (see ECustomBoneAttributeLookup)
       * @return Whether or not the atttribute was successfully retrieved
      */
      UFUNCTION(BlueprintCallable, Category = CustomAttributes)
      ENGINE_API bool GetFloatAttribute(const FName& BoneName, const FName& AttributeName, float DefaultValue, float& OutValue, ECustomBoneAttributeLookup LookupType = ECustomBoneAttributeLookup::BoneOnly);

      /**
       * Get FTransform type attribute value.
      
       * @param BoneName Name of the bone to retrieve try and retrieve the attribute from
       * @param AttributeName Name of the attribute to retrieve
       * @param OutValue (reference) Retrieved attribute value if found, otherwise is set to DefaultValue
       * @param LookupType Determines how the attribute is retrieved from the specified BoneName (see ECustomBoneAttributeLookup)
       * @return Whether or not the atttribute was successfully retrieved
      */
      UFUNCTION(BlueprintCallable, Category = CustomAttributes)
      ENGINE_API bool GetTransformAttribute(const FName& BoneName, const FName& AttributeName, FTransform DefaultValue, FTransform& OutValue, ECustomBoneAttributeLookup LookupType = ECustomBoneAttributeLookup::BoneOnly);
      
      /** 
       * Get integer type attribute value.
      
       * @param BoneName Name of the bone to retrieve try and retrieve the attribute from
       * @param AttributeName Name of the attribute to retrieve
       * @param DefaultValue In case the attribute could not be found
       * @param OutValue Retrieved attribute value if found, otherwise is set to DefaultValue
       * @param LookupType Determines how the attribute is retrieved from the specified BoneName (see ECustomBoneAttributeLookup)
       * @return Whether or not the atttribute was successfully retrieved
      */
      UFUNCTION(BlueprintCallable, Category = CustomAttributes)
      ENGINE_API bool GetIntegerAttribute(const FName& BoneName, const FName& AttributeName, int32 DefaultValue, int32& OutValue, ECustomBoneAttributeLookup LookupType = ECustomBoneAttributeLookup::BoneOnly);
    	
      /** 
    	 * Get string type attribute value.
    
    	 * @param BoneName Name of the bone to retrieve try and retrieve the attribute from
    	 * @param AttributeName Name of the attribute to retrieve
    	 * @param DefaultValue In case the attribute could not be found
         * @param OutValue Retrieved attribute value if found, otherwise is set to DefaultValue
    	 * @param LookupType Determines how the attribute is retrieved from the specified BoneName (see ECustomBoneAttributeLookup)
    	 * @return Whether or not the atttribute was successfully retrieved
    	*/
    	UFUNCTION(BlueprintCallable, Category = CustomAttributes)
    	ENGINE_API bool GetStringAttribute(const FName& BoneName, const FName& AttributeName, FString DefaultValue, FString& OutValue, ECustomBoneAttributeLookup LookupType = ECustomBoneAttributeLookup::BoneOnly);
    
    protected:
    	// Templated version to try and retrieve a typed bone attribute's value 
    	template<typename DataType, typename CustomAttributeType>
    	bool FindAttributeChecked(const FName& BoneName, const FName& AttributeName, DataType DefaultValue, DataType& OutValue, ECustomBoneAttributeLookup LookupType);	

    public:
	// Used to scale speed of all animations on this skeletal mesh. 
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=Animation, meta=(EditCondition = bEnableAnimation, ClampMin = 0.f))
	float GlobalAnimRateScale;
	
	// If we are running physics, should we update non-simulated bones based on the animation bone positions. 
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=SkeletalMesh)
	TEnumAsByte<EKinematicBonesUpdateToPhysics::Type> KinematicBonesUpdateType;
	
	// Whether physics simulation updates component transform. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Physics)
	TEnumAsByte<EPhysicsTransformUpdateMode::Type> PhysicsTransformUpdateMode;
	
	// whether we need to teleport cloth. 
	UPROPERTY(Interp, Transient, BlueprintReadOnly, VisibleAnywhere,  Category=Clothing) // This property is explicitly hidden from the details panel inside FSkeletalMeshComponentDetails::UpdatePhysicsCategory
	EClothingTeleportMode ClothTeleportMode;

    protected:
	// Whether to use Animation Blueprint or play Single Animation Asset. 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation, meta=(EditCondition = bEnableAnimation))
	TEnumAsByte<EAnimationMode::Type>	AnimationMode;
    public:
	// helper function to get the member name and verify it exists, without making it public
	static ENGINE_API const FName& GetAnimationModePropertyNameChecked();

	// helper function to get the member name and verify it exists, without making it public
    #if WITH_EDITORONLY_DATA
	static ENGINE_API FName GetSkeletalMeshAssetPropertyNameChecked();
    #endif // WITH_EDITORONLY_DATA

    private:
	// Teleport type to use on the next update 
	ETeleportType PendingTeleportType;

	/** Controls whether or not this component will evaluate its post process instance. The post-process
	 *  Instance is dictated by the skeletal mesh so this is used for per-instance control.
	 */
	UPROPERTY(EditAnywhere, BlueprintGetter=GetDisablePostProcessBlueprint, BlueprintSetter=SetDisablePostProcessBlueprint, Category = Animation, meta=(EditCondition = bEnableAnimation))
	uint8 bDisablePostProcessBlueprint:1;

     public:
	// Indicates that simulation (if it's enabled) is entirely responsible for children transforms. This is only ok if you are not animating attachment points relative to the simulation 
	uint8 bSimulationUpdatesChildTransforms:1;

	// Controls whether blending in physics bones will refresh overlaps on this component, defaults to true but can be disabled in cases where we know anim->physics blending doesn't meaningfully change overlaps 
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Physics)
	uint8 bUpdateOverlapsOnAnimationFinalize:1;

	// Temporary fix for local space kinematics. This only works for bodies that have no constraints and is needed by vehicles. Proper support will remove this flag 
	uint8 bLocalSpaceKinematics:1;

	// If true, there is at least one body in the current PhysicsAsset with a valid bone in the current SkeletalMesh 
	UPROPERTY(transient)
	uint8 bHasValidBodies:1;


	// Forces use of the physics bodies irrespective of whether they are simulated or using the physics blend weight 
	UPROPERTY(transient)
	uint8 bBlendPhysics:1;

	/**
	 *  If true, simulate physics for this component on a dedicated server.
	 *  This should be set if simulating physics and replicating with a dedicated server.
	 *	Note: This property cannot be changed at runtime.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = SkeletalMesh)
	uint8 bEnablePhysicsOnDedicatedServer:1;

	/** 
	 * If true, then the physics bodies will be used to drive the skeletal mesh even when they are
	 * kinematic (not simulating), otherwise the skeletal mesh will be driven by the animation input 
	 * when the bodies are kinematic
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = SkeletalMesh)
	uint8 bUpdateMeshWhenKinematic:1;

	/**
	 *	If we should pass joint position to joints each frame, so that they can be used by motorized joints to drive the
	 *	ragdoll based on the animation.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=SkeletalMesh)
	uint8 bUpdateJointsFromAnimation:1;

	/**
	 * Toggles creation of cloth simulation. Distinct from the simulation toggle below in that, if off, avoids allocating
	 * the actors entirely instead of just skipping the simulation step.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Clothing)
	uint8 bAllowClothActors:1;

	//  Disable cloth simulation and play original animation without simulation 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Clothing)
	uint8 bDisableClothSimulation:1;

	// Indicates that this SkeletalMeshComponent has deferred kinematic bone updates until next physics sim if not INDEX_NONE. 
	int32 DeferredKinematicUpdateIndex;
