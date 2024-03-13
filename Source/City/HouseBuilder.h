	// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "ProcMeshActor.h"
#include "ApartmentSpecification.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ThreadedWorker.h"
#include "atomic"
#include "HouseBuilder.generated.h"

class ThreadedWorker;

UCLASS()
class CITY_API AHouseBuilder : public AActor
{
	GENERATED_BODY()

	FHousePolygon f;
	float floorHeight = 400.0;

	int makeInterestingAttempts = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = generation, meta = (AllowPrivateAccess = "true"))
	float maxChangeIntensity = 0.35;

	bool generateRoofs = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = performance, meta = (AllowPrivateAccess = "true"))
	GenerationMode generationMode;

	unsigned int maxThreads = 1;

	bool shellOnly = false;

	bool wantsToWork = false;
	bool isWorking = false;
	int currentIndex = 0;
	int meshesPerTick = 3;
	TArray<FMeshInfo> meshesToPlace;
	TArray<UTextRenderComponent*> texts;

public:
	static std::atomic<unsigned int> housesWorking;

	static std::atomic<unsigned int> workersWorking;

	// Sets default values for this actor's properties
	AHouseBuilder();
	~AHouseBuilder();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = meshes, meta = (AllowPrivateAccess = "true"))
		TMap<FString, UHierarchicalInstancedStaticMeshComponent*> map;



	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = ProcMesh)
		TSubclassOf<class AProcMeshActor> procMeshActorClass;

	AProcMeshActor *procMeshActor;
	bool firstTime = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Info)
		FHousePolygon housePol;


	UFUNCTION(BlueprintCallable, Category = "Generation")
		void init(FHousePolygon f_in, float floorHeight_in, int makeInterestingAttempts_in, bool generateRoofs_in, GenerationMode generationMode_in){
		f = f_in;
		floorHeight = floorHeight_in;
		makeInterestingAttempts = makeInterestingAttempts_in;
		floorHeight = floorHeight_in;
		generateRoofs = generateRoofs_in;
		generationMode = generationMode_in;
		switch (generationMode) {
		case GenerationMode::complete: maxThreads = 1000; meshesPerTick = 1000; break;
		case GenerationMode::procedural_aggressive: maxThreads = 1000; meshesPerTick = 50; break;
		case GenerationMode::procedural_relaxed: maxThreads = 1; meshesPerTick = 5; break;
		}
	}

	UFUNCTION(BlueprintCallable, Category = "Generation")
		void setGenerationMode( GenerationMode generationMode_in) {
		generationMode = generationMode_in;
		switch (generationMode) {
		case GenerationMode::complete: maxThreads = 1000; meshesPerTick = 1000; break;
		case GenerationMode::procedural_aggressive: maxThreads = 1000; meshesPerTick = 50; break;
		case GenerationMode::procedural_relaxed: maxThreads = 1; meshesPerTick = 5; break;
		}
	}


	UFUNCTION(BlueprintCallable, Category = "Generation")
	FHouseInfo getHouseInfo();

	UFUNCTION(BlueprintCallable, Category = "Generation")
	void buildHouse(bool shellOnly);

	UFUNCTION(BlueprintCallable, Category = "Generation")
	void buildHouseFromInfo(FHouseInfo res);

	static void makeInteresting(FHousePolygon &f, TArray<FSimplePlot> &toReturn, FPolygon &centerHole, FRandomStream stream);

	static FPolygon getShaftHolePolygon(FHousePolygon f, FRandomStream stream, bool useCenter = false);

	ThreadedWorker *worker;

	bool workerWorking = false;
	bool workerWantsToWork = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	

public:	


	// Called every frame
	virtual void Tick(float DeltaTime) override;

	
	
};

