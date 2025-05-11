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

    private:
	// Disable rigid body animation nodes and play original animation without simulation 
	UPROPERTY(EditAnywhere, Category = Physics)
	uint8 bDisableRigidBodyAnimNode:1;

	// Disable animation curves for this component. If this is set true, no curves will be processed 
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = SkeletalMesh)
	uint8 bAllowAnimCurveEvaluation : 1;

	#if WITH_EDITORONLY_DATA
		// DEPRECATED. Use bAllowAnimCurveEvaluation instead 
		UE_DEPRECATED(4.18, "This property is deprecated. Please use bAllowAnimCurveEvaluatiuon instead. Note that the meaning is reversed.")	
		UPROPERTY()
		uint8 bDisableAnimCurves_DEPRECATED : 1;
	#endif

	// Whether or not we're taking cloth sim information from our leader component 
	uint8 bBindClothToLeaderComponent:1;

	// Flag denoting whether or not the clothing transform needs to update 
	uint8 bPendingClothTransformUpdate:1;

	// Flag denoting whether or not the clothing collision needs to update from its physics asset 
	uint8 bPendingClothCollisionUpdate:1;

    public:
	// can't collide with part of environment if total collision volumes exceed 16 capsules or 32 planes per convex 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Clothing)
	uint8 bCollideWithEnvironment:1;
	// can't collide with part of attached children if total collision volumes exceed 16 capsules or 32 planes per convex 
 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Clothing)
	uint8 bCollideWithAttachedChildren:1;
	/**
	 * Forces the cloth simulation to constantly update its external collisions, at the expense of performance.
	 * This will help to prevent missed collisions if the cloth's skeletal mesh component isn't moving,
	 * and when instead, wind or other moving objects are causing new collision shapes to come into the cloth's vicinity.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Clothing)
	uint8 bForceCollisionUpdate : 1;
	
	/**
	 * Scale applied to the component induced velocity of all cloth particles prior to stepping the cloth solver.
	 * Use 1.0 for fully induced velocity (default), or use 0.0 for no induced velocity, and any other values in between for a reduced induced velocity.
	 * When set to 0.0, it also provides a way to force the clothing to simulate in local space.
	 * This value multiplies to individual cloth's velocity scale settings, still allowing for differences in behavior between the various pieces of clothing within the same component.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Clothing, Meta = (UIMin = 0.f, UIMax = 1.f, ClampMin = 0.f, ClampMax = 1.f))
	float ClothVelocityScale = 1.f;
	
	// reset the clothing after moving the clothing position (called teleport) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Clothing)
	uint8 bResetAfterTeleport:1;
	
	// To save previous state 
	uint8 bPrevDisableClothSimulation : 1;
	
	/** 
	 * Optimization
	 */
	
	 // Whether animation and world transform updates are deferred. If this is on, the kinematic bodies (scene query data) will not update until the next time the physics simulation is run 
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category = SkeletalMesh)
	uint8 bDeferKinematicBoneUpdate : 1;
	
	// Skips Ticking and Bone Refresh. 
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=SkeletalMesh)
	uint8 bNoSkeletonUpdate:1;
	
	// pauses this component's animations (doesn't tick them, but still refreshes bones) 
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=Animation, meta=(EditCondition = bEnableAnimation))
	uint8 bPauseAnims:1;
	
	/**
	 * Whether the built-in animation of this component should run when the component ticks.
	 * It is assumed that if this is false then some external system will be animating this mesh.
	 * Note that disabling animation will also cause cloth simulation not to run and the component's tick to run on any thread. 
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=Animation)
	uint8 bEnableAnimation : 1;
	
	// On InitAnim should we set to ref pose (if false use first tick of animation data). If enabled, takes precedence over UAnimationSettings::bTickAnimationOnSkeletalMeshInit
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Animation, meta=(EditCondition = bEnableAnimation))
	uint8 bUseRefPoseOnInitAnim:1;
	
	/**
	* Uses skinned data for collision data.
	*/
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=SkeletalMesh)
	uint8 bEnablePerPolyCollision:1;
	
	/**
	 * Misc 
	 */
	
	// If true, force the mesh into the reference pose - is an optimization. 
	UPROPERTY()
	uint8 bForceRefpose:1;
	
	/** If true TickPose() will not be called from the Component's TickComponent function.
	* It will instead be called from Autonomous networking updates. See ACharacter. */
	UPROPERTY(Transient)
	uint8 bOnlyAllowAutonomousTickPose : 1;
	
	// True if calling TickPose() from Autonomous networking updates. See ACharacter. 
	UPROPERTY(Transient)
	uint8 bIsAutonomousTickPose : 1;
	
	// If bForceRefPose was set last tick. 
	UPROPERTY()
	uint8 bOldForceRefPose:1;
	
	// Bool that enables debug drawing of the skeleton before it is passed to the physics. Useful for debugging animation-driven physics. 
	UPROPERTY()
	uint8 bShowPrePhysBones:1;
	
	// If false, indicates that on the next call to UpdateSkelPose the RequiredBones array should be recalculated. 
	UPROPERTY(transient)
	uint8 bRequiredBonesUpToDate:1;
	
	// If true, AnimTree has been initialised. 
	UPROPERTY(transient)
	uint8 bAnimTreeInitialised:1;
	
	// If true, line checks will test against the bounding box of this skeletal mesh component and return a hit if there is a collision. 
	UPROPERTY()
	uint8 bEnableLineCheckWithBounds:1;
	
	// If true, propagates calls to ApplyAnimationCurvesToComponent for follower components, only needed if follower components do not tick themselves 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LeaderPoseComponent)
	uint8 bPropagateCurvesToFollowers : 1;

	#if WITH_EDITORONLY_DATA
		UE_DEPRECATED(5.1, "This property is deprecated. Please Use bPropagateCurvesToFollowers instead")
		unit8 bPropagateCurvesToSlaves : 1;
	#enif //WITH_EDITORONLY_DATA

	//Whether to skip UpdateKinematicBonesToAnim() when interpolating. Kinematic bones are updated to the target interpolation pose only on ticks when thy are evaluated. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Optimization)
	uint8 bSkipkinematicUpdateWhenInterpolating:1;

	//Whether to skip boundsupdate when interpolating. Bounds are updated to the target interpolation pose only on ticks when they are evaluated
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Optimization)
	unit8 bSkipBoundsUpdateWhenInterpolating:1;

    protected:

	// Whether the clothing simulation is suspended (not the same as disabled, we no longer run the sim but keep the last valid sim data around) 
	uint8 bClothingSimulationSuspended:1;

    #if WITH_EDITORONLY_DATA
	// If true, this will Tick until disabled 
	UPROPERTY(AdvancedDisplay, EditInstanceOnly, transient, Category = SkeletalMesh)
	uint8 bUpdateAnimationInEditor : 1;
	// If true, will play cloth in editor 
	UPROPERTY(AdvancedDisplay, EditInstanceOnly, transient, Category = SkeletalMesh)
	uint8 bUpdateClothInEditor : 1;

	// If true, DefaultAnimatingRigOverride will be used. If false, use the DefaultAnimatingRig in the SkeletalMesh 
	UPROPERTY(AdvancedDisplay,EditAnywhere, Category = SkeletalMesh, meta = (InlineEditConditionToggle))
	uint8 bOverrideDefaultAnimatingRig : 1;

    #endif

	// If true, OnSyncComponentToRBPhysics() notify will be called 
	uint8 bNotifySyncComponentToRBPhysics : 1;

    private:

	UPROPERTY(Transient)
	uint8 bNeedsQueuedAnimEventsDispacted:1;

	unit8 bPostEvalutatingAnimation:1;

    public:
	//Physcics-enigine representation of aggrgate which contains aphysics assest instance with more than numbers of bodies
	FPhysicsAggregateHandle Aggregate;

    public:

	//Cache ANimCurveUidVersion from Skeleton and this will be used to identify if it needs to be updated
	UPROPERTY(transient)
	uint16 CachedAnimCuriveUidVersion;

	/*
	*weight to blend between simulated results and key-framed positions
	*if weight if 1.0, shows only cloth simulation results and 0.0 will show only skinned results
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = Clothing)
	float ClothBlendWeight;

	/* Whether we should stall the Cloth tick task until the cloth simulation is complete. This is required if we want to up-to-date
	*cloth data on the game thread, for example, if we want to generate particles at cloth vertices
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cloting)
	bool bWaitForParallelClothTask;

    private:

	//Whether the FilteredAnimCurces list is an allow or deny list
	UPROPERTY(transient)
	bool bFilteredAnimCurvesIsAllowList = false;

	//Cache curve metadaa from mesh and this will be used to identify if it needs to be updated
	UPROPERTY(transient)
	uint16 CachedMeshCurveMetaDataVersion; 

	/*You can choose to disabl certain cirvs if you prefer
	*This is transint curves that will be ignored by animation systems if you choose this*/
	UPROPERTY(transient)
	TArray<FName> FilteredAnimCurves

    public: 

	/*
	*Used for per-poly collision. In 99% of cases, you will be better off using a Physics Asset.
	*This BodySetup is per instance because all modification of vertices is done in this place*/
	UPROPERTY(transient)
	TObjectPtr<class UBodySetup> BodySetup;

	ENGINE_API void CreateBodySetup();

	#if UE_ENABLED_DEBUG_DRAWING
		ENGINE_API virtual void SendRender DebugPhysics(FPrimitiveSceneProxy* OverrideSceneProxy = nullptr) override;
	#endif

    public: 
	
	//Threshold for physics asset bodies abov which we use an aggregate foro boardphase collisions
	int32 RagdollAggregateThreshold;

	UPROPERTY(Interp, BlueprintReadWrite, Category = Clothing, meta = (UIMin = 0.0, UIMax = 10.0, ClampMin = 0.0, ClampMax = 10000.0))
	float ClothMaxDistanceScalel

	/** This scale is applied to all cloth geometry (e.g, cloth meshes and collisions) in ordre to simulate  in a different scale space than world. This scale is not applied to distance-based simulation parameters such as MaxDistance
	*This proprty is currently only read by the cloth solver when creating cloth actor, by may become animatable in the futeure
	**/ 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = Clothing, meta = (UIMin = 0.0, UIMax = 10.0, ClampMin = 0.0, ClampMax = 10000.0))
	float ClothGeometryScale = 1.f;

    private: 
	/**
	* Max LOD level that is post-process AnimBPs are evaluated. Ovrrides the setting of the same name on the skeletal mesh. 
	*For examply, if you have the threshold set to 2, it will evaluat until including LOD 2 (based on 0 index). In cast the LOD level gets set to 3, it will stop evaluating the post-process AnimBP. 
	*Setting it to -1 will always evaluate it an disable LODing overrides for this component.
	**/
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = Animation, meta = (DisplayName = "Post-Process AnimBP LOD Threshold", AllowPrivateAccess = "true"))
	int32 PostProcessAnimBPLODThreshold = INDEX_NONE;

    public: 
	//Notification when constriant is broken
	UPROPERTY(BlueprintAssignable)
	FConstraintBrokenSignature OnConstraintBroken;

	//Notification when constraint plasticity drive target changes
	UPROPERTY(BlueprintAssignable)
	FPlasticDformationEventSignature OnPlasticDeformation; 

	//Class of the object responsible for 
	UPROPERTY(EditAnywhere, Category = Clothing)
	TSubOf<class UClothingSimulationFactory> ClothingSimulationFactory; 

	struct FPendingRadalForces
	{
		enum EType
		{
			AddImpluse, 
			AddForce, 
		};

		FVector Origin; 
		float Radius; 
		float Strength;
		ERadialImpulseFalloff Fallof; 
		bool bIgnoreMass;
		EType Type; 
		int32 FrameNum;

		FPendingRadialForces(FVector InOrigin, float InRadius, float InStrength, ERadialImpluseFallof InFalloff, bool InIngoreMass, EType InType)
			: Origin(InOrigin)
			, Radius(InRadius)
			, Strength(InStrength)
			, Falloff(InFalloff)
			, bIgnoreMass(InIgnoreMass)
			, Type(InType)
			, FrameNum(GFrameNumber)
		{	
		}
	};

	const TArray<FPendingRadialForces>& GetPendingRadialForces() const
	{
		return PendingRadialForces;	
	}
	//Array of physical interactions for the frame. This is a temporary solution for a more permanent force system and should not be used directly
	TArray<FPendingRadialForces> PendingRadialForces;

	UE_DEPRECATED(4.23, "This function is deprecated. Please use SetAnimInstaceClass instead.")
	ENGINE_API virtual void K2_SetAnimInstaceClass(class UClass* NewClass);

	//Set the anim instance class. Clears and re-initializes the anim instance with the nw class and sets animation mode to "AnimationBlueprint"
	UE_DEPRECATED(5.5, "This function is deprecated. Please use 'SetAnimInstanceClass' instead")
	ENGINE_API virtual void SetAnimClass(class UClass* NewClass);

	//Get the anim instance class via getter callable by sequencer
	UFUNCTION(BlueprintInstanceUseOnly)
	ENGINE_API class UClass* GetAnimClass();

	//Set the anim instance class. Clears and re-initalizes the anim instance with the nwe class and sets animation mode to "AnimationBlueprint"
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh", meta = (Keywords = "AnimBlueprint", DisplayName = "Set Anim Instance Class"))
	ENGINE_API virtual void SetAnimInstanceClass(class UClass* NewClass);

	/**
	*Returns the animation instance that is driving the class (if availabl)e). This is typcially an instance of
	*the class set as AnimBlueprintGeneratedClass (generatd by an animation blueprint)
	*Since this instance is transient, it is not safe to be used during construction script
	**/
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh", meta = (Keywords = "AnimBlueprint", UnsafeDuringActorConstruction = "true"))
	ENGINE_API class UAnimInstance* GetAnimInstance() const;

    private:
	//Setter function for anim instance class 
	void SetAnimInstanceClass_Internal(const TSubclassOf<UAnimInstance>& InAnimClass);

    public: 

	/**
	* Returns the active post-process instance if on is available. This is set on the mesh that this
	* component is using, and is evaluated immediately after the main instance
	**/
	UFUNCTION(BlueprintCallable, Captegory = "Components|SkeletalMesh", meta = (Keywords = "AnimBlueprint", UnsafeDuringActorConstruction = "true"))
	ENGINE_API UAnimInstance* GetPostProcessInstance() const;

	//Get the anim instances lined to the main AnimScriptInstance
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	const TArray<UAnimInstance*>& GetLinkdAnimInstances() const {return LinkedInstances;}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	//If true, next time, that ConditionallyDispatchdQueuedAnimEvents() is calld it will trigger any queued animn notifies available
	ENGINE_API void AllowQueuedAnimEventsNextDispatch();

    private: 
	PRAGMA_DISABLE_DEPERECATION_WARNINGS
	TArray<TObjectPtr<UAnimInstance>>& GtLinkedAnimInstances {return LinkedInstances;}
	ENGINE_API PRAGMA_ENABLE_DEPRECATION_WARNINGS

	//Clear the linked anim instancs and mark them pending kill
	void ResetLinkedAnimInstances();

	//Intrnal helper -- copis the mesh's reference pose to th local space. Transforms and regenerates component space transforms accordingly
	void ResetToRefPose();

    public: 
	UE_DEPRECATED(4.23, "This function is dprecated. Please use GetLinkedAnimGraphInsanceByTag")
	UAnimInstance* GetSubInstanceByName(FName InTag) const {return GetLinkedAnimGraphInstanceByTag(InTag);}

	UE_DEPRECATED(4.24, "This function is deprcated. Please use GetLinkedAnimGraphInstanceByTag")
	UAnimInstance* GetSubInstanceByag(FName InTag) const {return GetLinkedAnimGraphInstanceByTag(InTag);}

	/**
	* Returns th tagged linked instance node. If no linked instances are found, or none are tagged with the 
	* supplied name, this will return NULL
	**/ 
	UFUNCTION(BlueprintPure, Category = "Components|SkeletalMesh|Animation Blueprint Linking", meta = (Keywords = "Blueprint", UnsafeDuringActorConstruction = "true"))
	ENGINE_API UAnimInstance* GetLinkedAnimGraphInstanceByTag(FName InTag) const;

	UE_DEPRECATED(4.24, "Function renamed, please use GetLinkedAnimGraphInstanceByTag")
	void GetSubInstancesByTag(FName InTag, TArray<UAnimInstance*>& OutSubInstances) const
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		GetLinkedAnimGraphInstancesByTag(InTag, OutSubInstances);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	/**
	* Returns all tagged linked instanc modes that match the tag.
	**/
	UE_DEPRECATED(5.0, "Tags are unique so this funciton is no longer supported. Please use GetLinkedAnimGraphInstanceByTag instead")
	UFUNCTION(BlueprintPure, Category = "Components|SkeletalMesh|Animation Blueprint Linking", meta = (Keywords = "AnimBlueprint", DeprecatedFunction, DeprecationMessage="Tags are unique so this function is no longer supported. Please use GetLinkedAnimGraphInstanceByTag instead"))
	ENGINE_API void GetLinkedAnimGraphInstancesByTag(FName InTag, TArray<UAnimInstance*>& OutLinkedInstances) const;
	
	UE_DEPRECATED(4.24, "Function renamed, please use LinkAnimGraphByTag")
	void SetSubInstanceClassByTag(FName InTag, TSubclassOf<UAnimInstance> InClass) { LinkAnimGraphByTag(InTag, InClass); }
	
	// Runs through all nodes, attempting to find linked instance by name/tag, then sets the class of each node if the tag matches */
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh|Animation Blueprint Linking", meta = (Keywords = "AnimBlueprint", UnsafeDuringActorConstruction = "true"))
	ENGINE_API void LinkAnimGraphByTag(FName InTag, TSubclassOf<UAnimInstance> InClass);
	
	UE_DEPRECATED(4.24, "Function renamed, please use LinkAnimClassLayers")
	void SetLayerOverlay(TSubclassOf<UAnimInstance> InClass) { LinkAnimClassLayers(InClass); }

	/** 
	 * Runs through all layer nodes, attempting to find layer nodes that are implemented by the specified class, then sets up a linked instance of the class for each.
	 * Allocates one linked instance to run each of the groups specified in the class, so state is shared. If a layer is not grouped (ie. NAME_None), then state is not shared
	 * and a separate linked instance is allocated for each layer node.
	 * If InClass is null, then all layers are reset to their defaults.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh|Animation Blueprint Linking", meta = (Keywords = "AnimBlueprint", UnsafeDuringActorConstruction = "true"))
	ENGINE_API void LinkAnimClassLayers(TSubclassOf<UAnimInstance> InClass);
	
	UE_DEPRECATED(4.24, "Function renamed, please use UnlinkAnimClassLayers")
	void ClearLayerOverlay(TSubclassOf<UAnimInstance> InClass) { UnlinkAnimClassLayers(InClass); }
	
	/** 
	 * Runs through all layer nodes, attempting to find layer nodes that are currently running the specified class, then resets each to its default value.
	 * State sharing rules are as with SetLayerOverlay.
	 * If InClass is null, does nothing.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh|Animation Blueprint Linking", meta = (Keywords = "AnimBlueprint", UnsafeDuringActorConstruction = "true"))
	ENGINE_API void UnlinkAnimClassLayers(TSubclassOf<UAnimInstance> InClass);
	
	UE_DEPRECATED(4.24, "Function renamed, please use GetLinkedLayerInstanceByGroup")
	UAnimInstance* GetLayerSubInstanceByGroup(FName InGroup) const { return GetLinkedAnimLayerInstanceByGroup(InGroup); }
	
	// Gets the layer linked instance corresponding to the specified group 
	UFUNCTION(BlueprintPure, Category = "Components|SkeletalMesh|Animation Blueprint Linking", meta = (Keywords = "AnimBlueprint", UnsafeDuringActorConstruction = "true"))
	ENGINE_API UAnimInstance* GetLinkedAnimLayerInstanceByGroup(FName InGroup) const;

	UE_DEPRECATED(4.24, "Function renamed, please use GetLinkedAnimLayerInstanceByClass")
	UAnimInstance* GetLayerSubInstanceByClass(TSubclassOf<UAnimInstance> InClass) const { return GetLinkedAnimLayerInstanceByClass(InClass);  }
	
	// Gets the first layer linked instance corresponding to the specified class 
	UFUNCTION(BlueprintPure, Category = "Components|SkeletalMesh|Animation Blueprint Linking", meta = (Keywords = "AnimBlueprint", UnsafeDuringActorConstruction = "true"))
	ENGINE_API UAnimInstance* GetLinkedAnimLayerInstanceByClass(TSubclassOf<UAnimInstance> InClass) const;
	
	// Calls a function on each of the anim instances that this mesh component hosts, including linked and post-process instances 
	ENGINE_API void ForEachAnimInstance(TFunctionRef<void(UAnimInstance*)> InFunction);
	
	/** 
	 * Returns whether there are any valid instances to run, currently this means whether we have
	 * have an animation instance or a post process instance available to process.
	 */
	UFUNCTION(BlueprintPure, Category = "Components|SkeletalMesh", meta = (Keywords = "AnimBlueprint"))
	ENGINE_API bool HasValidAnimationInstance() const;
	
	/**
	 * Informs any active anim instances (main instance, linked instances, post instance) that a dynamics reset is required
	 * for example if a teleport occurs.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh", meta = (Keywords = "Dynamics,Physics", UnsafeDuringActorConstruction = "true"))
	ENGINE_API void ResetAnimInstanceDynamics(ETeleportType InTeleportType = ETeleportType::ResetPhysics);
	
	/** Below are the interface to control animation when animation mode, not blueprint mode */
	
	/**
	* Set the Animation Mode
	* @param InAnimationMode : Requested AnimationMode
	* @param bForceInitAnimScriptInstance : Init AnimScriptInstance if the AnimationMode is AnimationBlueprint even if the new animation mode is the same as current (this allows to use BP construction script to do this)
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Animation", meta = (Keywords = "Animation"))
	ENGINE_API void SetAnimationMode(EAnimationMode::Type InAnimationMode, bool bForceInitAnimScriptInstance = true);
	
	UFUNCTION(BlueprintPure, Category = "Components|Animation", meta = (Keywords = "Animation"))
	ENGINE_API EAnimationMode::Type GetAnimationMode() const;
	
	/* Animation play functions
	 *
	 * These changes status of animation instance, which is transient data, which means it won't serialize with this component
	 * Because of that reason, it is not safe to be used during construction script
	 * Please use OverrideAnimationData for construction script. That will override AnimationData to be serialized 
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|Animation", meta = (Keywords = "Animation", UnsafeDuringActorConstruction = "true"))
	ENGINE_API void PlayAnimation(class UAnimationAsset* NewAnimToPlay, bool bLooping);
	
	/* Animation play functions
	*
	* These changes status of animation instance, which is transient data, which means it won't serialize with this component
	* Because of that reason, it is not safe to be used during construction script
	* Please use OverrideAnimationData for construction script. That will override AnimationData to be serialized
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Animation", meta = (Keywords = "Animation", UnsafeDuringActorConstruction = "true"))
	ENGINE_API void SetAnimation(class UAnimationAsset* NewAnimToPlay);
	
	/* Animation play functions
	*
	* These changes status of animation instance, which is transient data, which means it won't serialize with this component
	* Because of that reason, it is not safe to be used during construction script
	* Please use OverrideAnimationData for construction script. That will override AnimationData to be serialized
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Animation", meta = (Keywords = "Animation", UnsafeDuringActorConstruction = "true"))
	ENGINE_API void Play(bool bLooping);
	
	/* Animation play functions
	*
	* These changes status of animation instance, which is transient data, which means it won't serialize with this component
	* Because of that reason, it is not safe to be used during construction script
	* Please use OverrideAnimationData for construction script. That will override AnimationData to be serialized
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Animation", meta = (Keywords = "Animation", UnsafeDuringActorConstruction = "true"))
	ENGINE_API void Stop();
	
	/* Animation play functions
	*
	* These changes status of animation instance, which is transient data, which means it won't serialize with this component
	* Because of that reason, it is not safe to be used during construction script
	* Please use OverrideAnimationData for construction script. That will override AnimationData to be serialized
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Animation", meta = (Keywords = "Animation", UnsafeDuringActorConstruction = "true"))
	ENGINE_API bool IsPlaying() const;
	
	/* Animation play functions
	*
	* These changes status of animation instance, which is transient data, which means it won't serialize with this component
	* Because of that reason, it is not safe to be used during construction script
	* Please use OverrideAnimationData for construction script. That will override AnimationData to be serialized
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Animation", meta = (Keywords = "Animation", UnsafeDuringActorConstruction = "true"))
	ENGINE_API void SetPosition(float InPos, bool bFireNotifies = true);
	
	/* Animation play functions
	*
	* These changes status of animation instance, which is transient data, which means it won't serialize with this component
	* Because of that reason, it is not safe to be used during construction script
	* Please use OverrideAnimationData for construction script. That will override AnimationData to be serialized
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Animation", meta = (Keywords = "Animation", UnsafeDuringActorConstruction = "true"))
	ENGINE_API float GetPosition() const;
	
	/* Animation play functions
	*
	* These changes status of animation instance, which is transient data, which means it won't serialize with this component
	* Because of that reason, it is not safe to be used during construction script
	* Please use OverrideAnimationData for construction script. That will override AnimationData to be serialized
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Animation", meta = (Keywords = "Animation", UnsafeDuringActorConstruction = "true"))
	ENGINE_API void SetPlayRate(float Rate);
	
	/* Animation play functions
	*
	* These changes status of animation instance, which is transient data, which means it won't serialize with this component
	* Because of that reason, it is not safe to be used during construction script
	* Please use OverrideAnimationData for construction script. That will override AnimationData to be serialized
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Animation", meta = (Keywords = "Animation", UnsafeDuringActorConstruction = "true"))
	ENGINE_API float GetPlayRate() const;

	/**
	* This ovrrides the current AnimationData parameter in the SkeletalMeshComponnt. This will serialize whn th component serialize
	* so it can be used during construction script. However, note that this will override current existing data
	* This can be useful if you'd like to mak a blueprint with custom default animation per component
	* This sets single player mode, which means you cannot use AnimBlueprint with it
	**/ 
	UFUNCTION(BlueprintCallable, Category = "Components|Animation", meta = (Keywords = "Animation"))
	ENGINE_API void OverrideAnimationData(UAnimationAsset* InAnimToPlay, bool bIsLooping = true, bool bIsPlaying = true, float Position = 0.f, float PlayRate = 1.f);

	/**
	* Set Morph Target with Name and Value(0-1)
	*
	* @param bRemoveZeroWeight : Used by editor code when it should stay in the active list with zero weight
	**/
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh", meta=(UnsafeDuringActorConstruction="true"))
	ENGINE_API void SetMorphTarget(FName MorphTargetName, float Value, bool bRemoveZeroWeight=true);

	/**
	* Clear all Morph Target that are st to this mesh
	**/ 
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	ENGINE_API void ClearMorphTargets();

	/** 
	* Get Morph target w/ the given name
	**/
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	ENGINE_API float GetMorphTarget(FName MorphTargetName) const;

	/** 
	* Takes a snapshot of this skeletal mesh component's pose and saves it to the specifiede snapshot
	* The snapshot is taken at the current LOD, so if for example, you ook th snapshot at LOD1
	* and then used it at LOD0 any bones not in LOD1 will use the reference pose
	**/ 
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh")
	ENGINE_API void SnapshotPose(UPARAM(ref) FPoseSnapshot& Snapshot);

	/** 
	* Sets whether cloth assets should be create/simulated in this component
	* This will update the conditional flag and you will want to call RecrateClothingActors for it to take effect. 
	* @param bInAllow Whether to allow the creation of cloth assests and simultion. 
	**/ 
	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	ENGINE_API void SetAllowClothActors(bool bInAllow);

	UFUNCTION(BlueprintCallable, Category="Components|SkeletalMesh")
	bool GetAllowClothActors() const { return bAllowClothActors; }

	/**
	* Get/Set the max distance scale of clothing mesh vertices
	**/
	UFUNCTION(BlueprintCallable, Category="Clothing")
	ENGINE_API float GetClothMaxDistanceScale() const;
	UFUNCTION(BlueprintCallable, Category="Clothing")
	ENGINE_API void SetClothMaxDistanceScale(float Scale);

	/** 
	* Used to indicate w should force 'teleport' during the next call to UpdateClothState, 
	* This will transform positions and vlocities and thus keep the simulation state, just translate it to a new pose
	**/
	UFUNCTION(BlueprintCallable, Category="Clothing")
	ENGINE_API void ForceClothNextUpdateTeleport();
	/**
	* Used to indicate we should force 'teleport adn reset' during the next call to UpdateClothState. 
	* This can be used to reset it form a bad state or by a teleport where the old state is not important anymore.
	**/
	UFUNCTION(BlueprintCallable, Category="Clothing")
	ENGINE_API void ForceClothNextUpdateTeleportAndReset();

	//Stops simulating clothing, but does not show clothing ref pose. Keeps the last known simulation state
	UFUNCTION(BlueprintCallable, Category="Clothing", meta=(UnsafeDuringActorConstruction))
	ENGINE_API void SuspendClothingSimulation();

	//Resumes a previously suspended clothing simulation, teleporting the clothing on the next tick
	UFUNCTION(BlueprintCallable, Category = "Clothing", meta=(UnsafeDuringActorConstruction))
	ENGINE_API void ResumeClothingSimulation();

	//Gets whether or not the clothing simulation is currently suspended
	UFUNCTION(BlueprintCallable, Category = "Clothing")
	ENGINE_API bool IsClothingSimulationSuspended() const;

	/** 
	* Reset the teleport mode of a next update to 'Continuous'
	**/
	UFUNCTION(BlueprintCallable, Category="Clothing")
	ENGINE_API void ResetClothTeleportMode();

	/**
	* If this component has a valid LeaderPoseComponent then this function makes cloth items on the follower component
	* take the transforms of the cloth items on the leader component instad of simulation separately. 
	* @Note This will FORCE any cloth actor on the leader component to simulate in local space. Also 
	* The meshes used in the components must be identical for the cloth to bind correctly
	**/ 
	UFUNCTION(BlueprintCallable, Category = "Clothing", meta = (UnsafeDuringActorConstruction = "true"))
	ENGINE_API void BindClothToLeaderPoseComponent();

	UE_DEPRECATED(5.1, "This method has been deprecated. Please use BindClothToLeaderPoseComponent instead.")
	void BindClothToMasterPoseComponent() { BindClothToLeaderPoseComponent(); }

	/**
	* If this component has a valid LeaderPoseComponent and has its cloth bound to the 
	* MCP, this function will unbind the cloth adn resume simulation. 
	* @param bRetoreSimulationSpace if true and the leader pose cloth was originally simulating in world
	* space, we will restore this setting. This will cause the leader component to reset which may be
	* undesirable
	**/ 
	UFUNCTION(BlueprintCallable, Category="Clothing", meta=(UnsafeDuringActorConstruction="true"))
	ENGINE_API void UnbindClothFromLeaderPoseComponent(bool bRestoreSimulationSpace = true);

	UE_DEPRECATED(5.1, "This method has been deprecated. Please use UnbindClothFromLeaderPoseComponent instead.")
	void UnbindClothFromMasterPoseComponent(bool bRestoreSimulationSpace = true) { UnbindClothFromLeaderPoseComponent(bRestoreSimulationSpace); }

	/** 
	* Sets whether or not to allow rigidy body animation node for this component
	**/ 
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh")
	ENGINE_API void SetAllowRigidBodyAnimNode(bool bInAllow, bool bReinitAnim = true);

	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh")
	bool GetAllowRigidBodyAnimNode() const { return !bDisableRigidBodyAnimNode; }

	/**
	* Sets whether or no to force ick component in order to update animation and refresh transform for this component
	* This is supporteed only in the editor
	**/
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh", meta = (DevelopmentOnly, UnsafeDuringActorConstruction = "true"))
	ENGINE_API void SetUpdateAnimationInEditor(const bool NewUpdateState);

	/**
	* Sets whether or not to animation cloth in the editor. Requires Animation In Editor to alos be true. 
	* This is supported only in the editor
	**/ 
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh", meta = (DevelopmentOnly, UnsafeDuringActorConstruction = "true"))
	ENGINE_API void SetUpdateClothInEditor(const bool NewUpdateState);

	#if WITH_EDITOR
		/**
		* return true if currently updating in editor is true
		* this is non BP because this is only used for follower componenet to detech elader component ticking state
		**/
		bool GetUpadteAnimationInEditor() const
		{
			return bUpdateAnimationInEditor;
		}

		bool GetUpdateClothEditor() const
		{
			return bUpdateClothInEditor; 
		}
	#endif

	UE_DEPRECATED(4.18, "This function is deprecated. Please use SetAllowAnimCurveEvaluation instead. Note that the meaning is reversed.")
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh")
	ENGINE_API void SetDisableAnimCurves(bool bInDisableAnimCurves);
	
	UE_DEPRECATED(4.18, "This function is deprecated. Please use GetAllowedAnimCurveEvaluate instead. Note that the meaning is reversed.")
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh")
	bool GetDisableAnimCurves() const { return !bAllowAnimCurveEvaluation; }
	
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh")
	ENGINE_API void SetAllowAnimCurveEvaluation(bool bInAllow);
	
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh")
	bool GetAllowedAnimCurveEvaluate() const { return bAllowAnimCurveEvaluation; }
	
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh")
	ENGINE_API void AllowAnimCurveEvaluation(FName NameOfCurve, bool bAllow);

	//By reset, it will allow all the curves to be evaluated
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh")
	ENGINE_API void ResetAllowedAnimCurveEvaluation();

	//Resets, and then only allow the following list to be allowed/disallowed
	UFUNCTION(BlueprintCallable, Category = "Components|SkeletalMesh")
	ENGINE_API void SetAllowedAnimCurvesEvaluation(const TArray<FName>& List, bool bAllow);

	UE_DEPRECATED(5.3, "Please use GetCurveFilterSettings")
	const TArray<FName>& GetDisallowedAnimCurvesEvaluation() const { return FilteredAnimCurves; }

	/** 
	* Get the curve settings that will be used for anim evaluation. 
	* @param InLODOverride	Override the LOD that curves will be calculated for, if not INDEX_NONE
	* @return the curve settings used for evaluation 
	**/ 
	ENGINE_API UE::Anim::FCurveFilterSettings GetCurveFilterSettings(int32 InLODOverride = INDEX_NONE) const;

	/** We detach the Component once we are done playing it. 
	* 
	* @param ParticleSystemComponent that finished
	**/ 
	ENGINE_API virtual void SkelMeshCompOnParticleSystemFinished( class UParticleSystemComponent* PSC);

	ENGINE_API class UAnimSingleNodeInstance * GetSingleNodeInstance() const;
	ENGINE_API bool InitializeAnimScriptInstance(bool bForceReinit = true, bool bInDeferRootNodeInitialization = false);

	// @return true if wind is enabled
	ENGINE_API virtual bool IsWindEnabled() const;

	#if WITH_EDITOR
		/** 
		* Subclass such as DebugSkelMeshComponent keep track of errors in the anim notifies so they can be displayed to the user. This function adds an error. 
		* Errors are added uniquely and only removed when they're cleard by ClearAnimNotifyError.
		**/ 
		virtual void ReportAnimNotifyError(const FText& Error, UObject* InSourceNotify){}

		/**
		* Clears currently stored errors. Call before triggering anim notifies for a particular mesh.
		*/
		virtual void ClearAnimNotifyErrors(UObject* InSourceNotify){}
	#endif

    public: 
	/** 
	* Index of the 'Root Body', or top body in the asset hierarchy. 
	* Filled in by InItInstance, so we don't need to save it. 
	**/
	//To save root body index/one index consistently
	struct 
	{
		int32 BodyIndex;
		FTransform TransformToRoot;
	} RootBodyData;

	//Set Root Body Index
	ENGINE_API void SetRootBodyIndex(int32 InBodyIndex);
	// Reset Root Body Index
	ENGINE_API void ResetRootBodyIndex();

	//Temporary array of bone indicies requird for this frame
	TArray<FBoneIndexType> RequiredBones;

	//Temporary array of bon indicies required to populate component space transforms
	TArray<FBoneIndexType> FillComponentSpaceTransformsRequiredBones;

	//Array of FBodyInstance objects, storing per-instance state about each body part
	TArray<struct FBodyInstance*> Bodies;

	//Array of FConstriantInstance structs, storing per-instance state about each constraint
	TArray<struct FConstraintInstance*> Constraints;

	FSkeletalMeshComponentClothTickFunction ClothTickFunction;

	/**
	* Gets the telportation rotation threshold. 
	* 
	* @return Threshold in degrees
	**/ 
	UFUNCTION(BlueprintGetter, Category=Clothing)
	ENGINE_API float GetTeleportRotationThreshold() const;

	/**
	* Sets the teleportation rotation threshold
	*
	* @param threshold Threshold in degrees
	**/ 
	UFUNCTION(BlueprintSetter, Category=Clothing)
	ENGINE_API void SetTeleportRotationThreshold(float Threshold);

	/**
	* Gets the teleportation distance threshold 
	*
	* @return Threshold value
	**/ 
	UFUNCTION(BlueprintGetter, Category=Clothing)
	ENGINE_API float GetTeleportDistanceThreshold() const;

	/** 
	* Sets the teleportation distance threshold. 
	* 
	* @param threshold Threshold value
	**/ 
	UFUNCTION(BlueprintSetter, Category=Clothing)
	ENGINE_API void SetTeleportDistanceThreshold(float Threshold);

    private: 
	/**
	* Conduct teleportation if the character's movemnet is greater tham this threshold in 1 frame
	* Zero or negative values will skip the check. 
	* You can also do force teleport manually using ForceNextUpdateTeleport() / ForceNextUpdateTeleportAndReset().
	**/ 
	UPROPERTY(EditAnywhere, BlueprintGetter=GetTeleportDistanceThreshold, BlueprintSetter=SetTeleportDistanceThreshold, Category=Clothing)
	float TeleportDistanceThreshold;

	/**
	* Rotation threshld in degrees, ranging from 0 to 100. 
	* Conduct telportation if the character's rotation is greater than this threshold in 1 frame
	* Zero or negatice values will skip the check
	**/ 
	UPROPERTY(EditAnywhere, BlueprintGetter=GetTeleportRotationThreshold, BlueprintSetter=SetTeleportRotationThreshold, Category=Clothing)
	float TeleportRotationThreshold;

	//Used for pre-computation using TeleportRotationThreshold property
	float ClothTeleportCosineThresholdInRad;
	//Used for pre-computation used tTeleportDistanceThreshold property
	float ClothTeleportDistThresholdSquared;

	ENGINE_API void ComputeTeleportRotationThresholdInRadians();
	ENGINE_API void ComputeTeleportDistanceThresholdInRadians();

    public: 

	//Checked whether we have already ticked this pose this frame
	ENGINE_API bool PoseTickedThisFrame() const;

	bool IsClothBoundToLeaderComponent() const { return bBindClothToLeaderComponent; }

	UE_DEPRECATED(5.1, "This method has been deprecated. Please, use IsClothBoundToLeaderComponent instead.")
	bool IsClothBoundToMasterComponent() const { return IsClothBoundToLeaderComponent(); }

	//Get the current clothing simulation (read-only)
	ENGINE_API const IClothingSimulation* GetClothingSimulation() const;

	//Get the current clothing simulation (read/write)
	ENGINE_API IClothingSimulation* GetClothingSimulation();

	//Get the current clothing simulation context (read only)
	ENGINE_API const IClothingSimulationContext* GetClothingSimulationContext() const;

	//Get the currnt clothing simulation context (read/write)
	ENGINE_API IClothingSimulationContext* GetClothingSimulationContext();

	//Get the currnt interactor for the clothing simulation, if the current simulation supports runtime interaction 
	UFUNCTION(BlueprintCallable, Category=Clothing)
	ENGINE_API UClothingSimulationInteractor* GetClothingSimulationInteractor() const;

	//Callback when the parallel clothing task finishes, copies needed data back to component for gamethread
	ENGINE_API void CompleteParallelClothSimulation();

	//Get th current simulation data mata for the clothing on this component. For use ont he game thread and only valid if bWaitForParallelClothTask is true
	ENGINE_API const TMap<int32, FClothSimulData>& GetCurrentClothingData_GameThread() const;

	//Get the current simulation data map for the clothing on this component. This will stall until th cloth simulation is complete
	ENGINE_API const TMap<int32, FClothSimulData>& GetCurrentClothingData_AnyThread() const;

	//Stalls on any currently running clothing simulations 
	ENGINE_API void WaitForExistingParallelClothSimulation_GameThread();

    private:

	//Let the cloth tick and completion tasks have access to private clothing data
	friend FSkeletalMeshComponentClothTickFunction;
	friend class FParallelClothCompletionTask;

	//Debug mesh component should be abl to access for visualization
	friend class UDebugSkelMeshComponent;

	//Copies the data from the external cloth simulation context. We copy instead of flipping because the API has to return the full struct to make backwards compat easy
	ENGINE_API void UpdateClothSimulationContext(float InDeltaTime);

	//Previous root bone matrix to compare the difference and decide to do clothing teleport
	FMatrix	PrevRootBoneMatrix;

	/**
	* Clothing simulation objects. 
	* ClothingSimulation is responsibl for maintaining and siulating clothing actors
	* ClothingSimulationContext is a datastore for simulation daa snet to the clothing thread
	**/ 
	IClothingSimulation* ClothingSimulation;
	IClothingSimulationContext* ClothingSimulationContext;

	/**
	* Object responsible for interacting with the clothing simulation 
	* Blueprints and code can call/set data on this from the game thread and the next time
	* it is safe to do so th interactor will sync to the simulation context
	**/ 
	UPROPERTY(Transient)
	TObjectPtr<UClothingSimulationInteractor> ClothingInteractor;

	//Array of sources of cloth collision 
	TArray<FClothCollisionSource> ClothCollisionSources;

	//Ref for the clothing parallel task, so we can detect whether or not a sim is running 
	FGraphEventRef ParallelClothTask;

	/** Whether we should stall the Cloth tick task until the cloth simulation is complete. This is required if we want up-to-date
	* cloth data on the game thread, for example, if we want to generate particles and cloth vertices. When the data is not required
	* except for rendering, we cn set this to false, to eliminate a potential game thread stall while we wait for the cloth sim
	**/
	ENGINE_API bool ShouldWaitForClothInTickFunction() const;

	//Stalls on any currently running clothing simulations, needed when changing core sim state, or to access the clothing data
	ENGINE_API void HandleExistingParallelClothSimulation();

	/** Called by the clothing completion event to perform a writeback of the simulation data
	* to the game thread, the task is friended to gain access to this and not allow any 
	* external callers to trigger writebacks
	**/ 
	ENGINE_API void WritebackClothingSimulationData();

	//Gets the factory responsibl for building the clothing simulation and simulation contexts
	ENGINE_API UClothingSimulationFactory* GetClothingSimFactory() const;

    protected:

	/** Simulation data written back to the componet after the simulation has taken place. If this data is required, 
	* by any system other than rendering, bWaitForParallelClothTask must b true. If bWaitForParallelClothTask is false
	* this data cannot be read on the game thread, and there must be a call to HandleExistingParallelCltohSimulation() prior
	* to accessing it. 
	**/
	TMap<int32, FClothSimulData> CurrentSimulationData;

    private:

	//Wrapper that calls our constraint broken delegate
	ENGINE_API void OnConstraintBrokenWrapper(int32 ConstraintIndex);

	//Wrapper htat calls our constraint plasiticity delegate
	ENGINE_API void OnPlasticDeformationWrapper(int32 ConstraintIndex);

	/**
	* Morph Target Curves. 
	* This will override AnimInstance MorphTargetCurves if same curve is found
	**/
	TMap<FName, float> MorphTargetCurves;
