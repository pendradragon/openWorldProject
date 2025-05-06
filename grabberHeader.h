// In header
UPROPERTY()
USceneComponent* Grabber;

UPROPERTY()
USphereComponent* GrabberTrigger;

UFUNCTION()
void OnGrabberOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                      bool bFromSweep, const FHitResult& SweepResult);

// In constructor
Grabber = CreateDefaultSubobject<USceneComponent>(TEXT("Grabber"));
RootComponent = Grabber;

GrabberTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("GrabberTrigger"));
GrabberTrigger->SetupAttachment(Grabber);
GrabberTrigger->InitSphereRadius(100.f);
GrabberTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
GrabberTrigger->OnComponentBeginOverlap.AddDynamic(this, &AYourPawn::OnGrabberOverlap);
