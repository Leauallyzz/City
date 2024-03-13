#include "City.h"
#include "NoiseSingleton.h"
#include "Spawner.h"
#include <ctime>


// Sets default values
ASpawner::ASpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}



int convertToMapIndex(int x, int y) {
	return x * 10000 + y;
}



void addRoadToMap(TMap <int, TArray<FRoadSegment*>*> &map, FRoadSegment* current, float intervalLength) {
	FVector middle = (current->p2 - current->p1) / 2 + current->p1;
	int x = FMath::RoundToInt((middle.X / intervalLength));
	int y = FMath::RoundToInt((middle.Y / intervalLength));
	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			if (map.Find(convertToMapIndex(i+x, j+y)) == nullptr) {
				TArray<FRoadSegment*>* newArray = new TArray<FRoadSegment*>();
				newArray->Add(current);
				map.Emplace(convertToMapIndex(i+x, j+y), newArray);
			}
			else {
				map[convertToMapIndex(i+x, j+y)]->Add(current);
			}
		}
	}

}

TArray<TArray<FRoadSegment*>*> getRelevantSegments(TMap <int, TArray<FRoadSegment*>*> &map, FRoadSegment* current, float intervalLength) {
	TArray<TArray<FRoadSegment*>*> allInteresting;
	FVector middle = (current->p2 - current->p1) / 2 + current->p1;
	int x = FMath::RoundToInt((middle.X / intervalLength));
	int y = FMath::RoundToInt((middle.Y / intervalLength));

	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			if (map.Find(convertToMapIndex(i+x, j+y)) != nullptr) {
				allInteresting.Add(map[convertToMapIndex(i+x, j+y)]);
			}
		}
	}

	return allInteresting;

}


void ASpawner::addVertices(FRoadSegment* road) {
	road->v1 = road->p1 + FRotator(0, 90, 0).RotateVector(road->beginTangent) * standardWidth*road->width / 2;
	road->v2 = road->p1 - FRotator(0, 90, 0).RotateVector(road->beginTangent) * standardWidth*road->width / 2;
	FVector endTangent = road->p2 - road->p1;
	endTangent.Normalize();
	road->v3 = road->p2 + FRotator(0, 90, 0).RotateVector(endTangent) * standardWidth*road->width / 2;
	road->v4 = road->p2 - FRotator(0, 90, 0).RotateVector(endTangent) * standardWidth*road->width / 2;

}


void ASpawner::collideInto(FRoadSegment *s1, FRoadSegment *s2, FVector impactP) {
	//FVector tangent2 = impactP - f2->p1;
	//tangent2.Normalize();
	s1->p2 = impactP;// f2->p1 + (len - standardWidth / 2) * tangent;

	FVector naturalTangent = s1->p2 - s1->p1;
	naturalTangent.Normalize();
	FVector pot1 = FRotator(0, 90, 0).RotateVector(s2->p2 - s2->p1);
	pot1.Normalize();
	FVector pot2 = FRotator(0, 270, 0).RotateVector(s2->p2 - s2->p1);
	pot2.Normalize();

	FVector potentialNewTangent;
	if (FVector::Dist(pot1, naturalTangent) < FVector::Dist(pot2, naturalTangent))//(FVector::DotProduct(pot1, naturalTangent) > 0.7)
		potentialNewTangent = pot1;
	else
		potentialNewTangent = pot2;


	// make sure new tangent is not completely off the rails, it's possible they collide from front
	if (FVector::DotProduct(naturalTangent, potentialNewTangent) > 0.3) {
		s1->endTangent = potentialNewTangent;
		addVertices(s1);
	}
	else {
		s1->endTangent = -s2->endTangent;
		s1->v3 = s2->v4;
		s1->v4 = s2->v3;
		s1->p2 = middle(s2->v4, s2->v3);
		s2->roadInFront = true;
	}
	
	s1->roadInFront = true;
}

bool ASpawner::placementCheck(TArray<FRoadSegment*> &segments, logicRoadSegment* current, TMap <int, TArray<FRoadSegment*>*> &map){

	// use SAT collision between all roads
	FVector tangent1 = current->segment->p2 - current->segment->p1;
	tangent1.Normalize();
	FVector tangent2 = FRotator(0, 90, 0).RotateVector(tangent1);

	addVertices(current->segment);
	TArray<FVector> vert1;
	vert1.Add(current->segment->v1);
	vert1.Add(current->segment->v2);
	vert1.Add(current->segment->v3);
	vert1.Add(current->segment->v4);

	for (FRoadSegment* f : segments){
		if (f == current->previous->segment)
			continue; 
		
		TArray<FVector> tangents;




		FVector tangent3 = f->p2 - f->p1;
		FVector tangent4 = FRotator(0, 90, 0).RotateVector(tangent3);

		tangent1.Normalize();
		tangent2.Normalize();
		tangent3.Normalize();
		tangent4.Normalize();
		// can't be too close to another segment
		bool closeMiddle = FVector::Dist((f->p2 + f->p1) / 2, (current->segment->p2 + current->segment->p1) / 2) < 4000;
		//bool closeOtherEnd = FVector::Dist(f->p2, current->segment->p2) < 3000;
		if (closeMiddle) {
			return false;
		}
		//if (closeOtherEnd && FVector::DotProduct(tangent3, tangent1) < -0.8f){
		//	return false;
		//}

		tangents.Add(tangent1);
		tangents.Add(tangent2);
		tangents.Add(tangent3);
		tangents.Add(tangent4);

		TArray<FVector> vert2;
		vert2.Add(f->v1);
		vert2.Add(f->v2);
		vert2.Add(f->v3);
		vert2.Add(f->v4);

		//if (testCollision(tangents, vert1, vert2, collisionLeniency)) {
		FVector newE = intersection(current->segment->p1, current->segment->p2, f->p1, f->p2);
		if (newE.X != 0) {
			current->time = 100000;
			collideInto(current->segment, f, newE);
		}



		//}


	}

	return true;
	

}

float getValueOfRotation(FVector testPoint, TArray<logicRoadSegment*> &others, float maxDist, float detriment) {
	float val = NoiseSingleton::getInstance()->noise(testPoint.X, testPoint.Y);//noise(noiseScale, testPoint.X, testPoint.Y);
	for (logicRoadSegment* t : others) {
		if (FVector::Dist(t->segment->getMiddle(), testPoint) < maxDist)
			val -= std::max(0.0f, detriment * (maxDist - FVector::Dist(t->segment->getMiddle(), testPoint))/maxDist);
	}
	return val;
}



FRotator getBestRotation(float maxDiffAllowed, FRotator original, FVector originalPoint, FVector step, TArray<logicRoadSegment*> &others, float maxDist, float detriment) {
	FVector testPoint = originalPoint + original.RotateVector(step);
	float bestVal = -10000;
	FRotator bestRotator = original;
	for (int i = 0; i < 7; i++) {
		FRotator curr = original + FRotator(0, baseLibraryStream.FRandRange(-maxDiffAllowed, maxDiffAllowed), 0);
		testPoint = originalPoint + curr.RotateVector(step);
		float val = getValueOfRotation(testPoint, others, maxDist, detriment);
		if (val > bestVal) {
			bestRotator = curr;
			bestVal = NoiseSingleton::getInstance()->noise(testPoint.X, testPoint.Y);
		}
	}
	return bestRotator;
}

void ASpawner::addRoadForward(std::priority_queue<logicRoadSegment*, std::deque<logicRoadSegment*>, roadComparator> &queue, logicRoadSegment* previous, std::vector<logicRoadSegment*> &allsegments) {
	FRoadSegment* prevSeg = previous->segment;
	logicRoadSegment* newRoadL = new logicRoadSegment();
	FRoadSegment* newRoad = new FRoadSegment();
	FVector stepLength = prevSeg->type == RoadType::main ? primaryStepLength : secondaryStepLength;

	newRoad->p1 = prevSeg->p2;

	TArray<logicRoadSegment*> others;
	if (prevSeg->type == RoadType::main) {
		for (logicRoadSegment* f : allsegments) {
			if (f->segment->type == RoadType::main) {
				others.Add(f);
			}
		}
	}


	FRotator bestRotator = getBestRotation((prevSeg->type == RoadType::main ? changeIntensity : secondaryChangeIntensity), previous->rotation,newRoad->p1, stepLength, others, mainRoadDetrimentRange, mainRoadDetrimentImpact);

	newRoadL->rotation = bestRotator;


	newRoad->p2 = newRoad->p1 + newRoadL->rotation.RotateVector(stepLength);
	newRoad->beginTangent = prevSeg->p2 - prevSeg->p1;
	newRoad->beginTangent.Normalize();
	newRoad->width = prevSeg->width;
	newRoad->type = prevSeg->type;
	newRoad->endTangent = newRoad->p2 - newRoad->p1;
	newRoadL->segment = newRoad;
	float val = getValueOfRotation(newRoad->p2, others, mainRoadDetrimentRange, mainRoadDetrimentImpact);
	newRoadL->time = -val + ((newRoad->type == RoadType::main) ? mainRoadAdvantage : 0) + std::abs(0.1*previous->time);// + baseLibraryStream.FRand() * 0.1;
	newRoadL->roadLength = previous->roadLength + 1;
	newRoadL->previous = previous;
	addVertices(newRoad);
	queue.push(newRoadL);
	allsegments.push_back(newRoadL);
	
}

void ASpawner::addRoadSide(std::priority_queue<logicRoadSegment*, std::deque<logicRoadSegment*>, roadComparator> &queue, logicRoadSegment* previous, bool left, float width, std::vector<logicRoadSegment*> &allsegments, RoadType newType) {
	FRoadSegment* prevSeg = previous->segment;
	logicRoadSegment* newRoadL = new logicRoadSegment();
	FRoadSegment* newRoad = new FRoadSegment();
	FVector stepLength = newType == RoadType::main ? primaryStepLength : secondaryStepLength;

	FRotator newRotation = left ? FRotator(0, 90, 0) : FRotator(0, 270, 0);
	newRoadL->rotation = previous->rotation + newRotation;
	FVector startOffset = newRoadL->rotation.RotateVector(FVector(standardWidth*previous->segment->width / 2, 0, 0));
	newRoad->p1 =prevSeg->p1 + (prevSeg->p2 - prevSeg->p1) / 2 + startOffset;


	TArray<logicRoadSegment*> others;
	if (newType == RoadType::main) {
		for (logicRoadSegment* f : allsegments) {
			if (f->segment->type == RoadType::main) {
				others.Add(f);
			}
		}
	}
	FRotator bestRotator = getBestRotation(secondaryChangeIntensity, newRoadL->rotation, newRoad->p1, stepLength, others, mainRoadDetrimentRange, mainRoadDetrimentImpact);
	newRoadL->rotation = bestRotator;


	newRoad->p2 = newRoad->p1 + newRoadL->rotation.RotateVector(stepLength);
	newRoad->beginTangent = FRotator(0, left ? 90 : 270, 0).RotateVector(previous->segment->p2 - previous->segment->p1); //->p2 - newRoad->p1;
	newRoad->beginTangent.Normalize();
	newRoad->width = width;
	newRoad->type = newType;
	newRoad->endTangent = newRoad->p2 - newRoad->p1;

	newRoadL->segment = newRoad;
	// every side track has less priority

	//FVector mP = middle(newRoad->p1, newRoad->p2);
	float val = getValueOfRotation(newRoad->p2, others, mainRoadDetrimentRange, mainRoadDetrimentImpact);
	newRoadL->time = -val + ((newRoad->type == RoadType::main) ? mainRoadAdvantage : 0) + std::abs(0.1*previous->time);// + baseLibraryStream.FRand() * 0.1;

	newRoadL->roadLength = (previous->segment->type == RoadType::main && newType != RoadType::main) ? 1 : previous->roadLength+1;
	newRoadL->previous = previous;

	addVertices(newRoad);
	queue.push(newRoadL);
	allsegments.push_back(newRoadL);

}

void ASpawner::addExtensions(std::priority_queue<logicRoadSegment*, std::deque<logicRoadSegment*>, roadComparator> &queue, logicRoadSegment* current, std::vector<logicRoadSegment*> &allsegments) {
	float mainRoadSize = 4.0f;
	float sndRoadMinSize = 1.9f;
	float sndRoadSize = std::max(sndRoadMinSize, current->segment->width - 1.5f);
	FVector tangent = current->segment->p2 - current->segment->p1;
	if (current->segment->type == RoadType::main) {
		// on the main road
		if (current->roadLength < maxMainRoadLength)
			addRoadForward(queue, current, allsegments);

		if (baseLibraryStream.FRandRange(0, 1) < mainRoadBranchChance)
			addRoadSide(queue, current, true, mainRoadSize, allsegments, RoadType::main);
		else
			addRoadSide(queue, current, true, sndRoadSize, allsegments, RoadType::secondary);
		if (baseLibraryStream.FRandRange(0, 1) < mainRoadBranchChance)
			addRoadSide(queue, current, false, mainRoadSize, allsegments, RoadType::main);
		else 
			addRoadSide(queue, current, false, sndRoadSize, allsegments, RoadType::secondary);

	}

	else if (current->segment->type == RoadType::secondary) {
		// side road
		if (current->roadLength < maxSecondaryRoadLength) {
			addRoadForward(queue, current, allsegments);

			addRoadSide(queue, current, true, sndRoadSize, allsegments, RoadType::secondary);
			addRoadSide(queue, current, false, sndRoadSize, allsegments, RoadType::secondary);
		}
	}


}

TArray<FRoadSegment> ASpawner::determineRoadSegments()
{

	std::clock_t begin = clock();



	// set the common random stream
	baseLibraryStream = stream;
		NoiseSingleton::getInstance()->setNoiseScale(noiseScale);

	if (useTexture) {
		NoiseSingleton::getInstance()->setUseTexture(noiseTexture, noiseTextureScale);
		NoiseSingleton::getInstance()->initForImage();
	}
	else {
		NoiseSingleton::getInstance()->initForPerlin(baseLibraryStream.RandRange(-10000, 10000), baseLibraryStream.RandRange(-10000, 10000));
	}

	// if we have no roof it looks better with polygons on side of walls as well, otherwise the top side of walls in the buildings will just be empty
	BaseLibrary::overrideSides = !generateRoofs;

	FVector origin;
	TArray<FRoadSegment*> determinedSegments;
	TArray<FRoadSegment> finishedSegments;


	std::priority_queue<logicRoadSegment*, std::deque<logicRoadSegment*>, roadComparator> queue;


	std::vector<logicRoadSegment*> allSegments;
	logicRoadSegment* start = new logicRoadSegment();
	start->time = -10000;
	FRoadSegment* startR = new FRoadSegment();

	FVector point = FVector(0, 0, 0);

	startR->p1 = point;


	start->segment = startR;
	float bestVal = -1000000;
	FRotator bestRot;
	for (int i = 0; i < 360; i++) {
		FVector testPoint = point + FRotator(0, i, 0).RotateVector(primaryStepLength);
		float testVal = NoiseSingleton::getInstance()->noise(testPoint.X, testPoint.Y);
		if (testVal > bestVal) {
			bestVal = testVal;
			bestRot = FRotator(0, i, 0);
		}
	}
	startR->p2 = startR->p1 + bestRot.RotateVector(primaryStepLength);
	startR->width = 4.0f;
	startR->type = RoadType::main;
	startR->endTangent = startR->p2 - startR->p1;
	startR->endTangent.Normalize();
	startR->beginTangent = startR->endTangent;
	start->rotation = bestRot;
	start->roadLength = 1;
	addVertices(startR);

	queue.push(start);

	// map for faster comparisons, unused currently
	TMap <int, TArray<FRoadSegment*>*> segmentsOrganized;

	// loop for everything else

	while (queue.size() > 0 && determinedSegments.Num() < length) {
		logicRoadSegment* current = queue.top();
		queue.pop();
		if (placementCheck(determinedSegments, current, segmentsOrganized)){/// && FVector::Dist(current->segment->p1, current->segment->p2) > 2000) {
			determinedSegments.Add(current->segment);
			if (current->previous && FVector::Dist(current->previous->segment->p2, current->segment->p1) < 1.0f)
				current->previous->segment->roadInFront = true;
			//addRoadToMap(segmentsOrganized, current->segment, primaryStepLength.Size() / 2);

			addExtensions(queue, current, allSegments);
		}
	}



	// make sure roads attach properly if there is another road not too far in front of them
	for (int k = 0; k < 1; k++) {
		for (int i = 0; i < determinedSegments.Num(); i++) {
			FRoadSegment* f2 = determinedSegments[i];
			if (f2->roadInFront)
				continue;
			FVector tangent = f2->p2 - f2->p1;
			tangent.Normalize();
			FVector p2Prev = f2->p2;
			FVector p1Prev = f2->p1;
			f2->p2 += tangent*maxAttachDistance;
			float closestDist = 10000000.0f;
			FRoadSegment* closest = nullptr;
			FVector impactP;
			for (int j = 0; j < determinedSegments.Num(); j++) {
				FRoadSegment* f = determinedSegments[j];
				if (i == j) {
					continue;
				}

				FVector fTan = f->p2 - f->p1;
				fTan.Normalize();
				FVector res = intersection(p2Prev, f2->p2, f->p1, f->p2);
				if (res.X != 0.0f && FVector::Dist(p2Prev, res) < closestDist) {
					closestDist = FVector::Dist(p2Prev, res);
					closest = f;
					impactP = res;
				}
			}
			if (closest) {
				collideInto(f2, closest, impactP);
			}
			else {
				f2->p2 = p2Prev;
			}
		}


	}


	TArray<int> keys;
	segmentsOrganized.GetKeys(keys);
	UE_LOG(LogTemp, Warning, TEXT("deleting %i keys in map"), keys.Num());
	for (int key : keys) {
		delete(segmentsOrganized[key]);
	}

	for (FRoadSegment* f : determinedSegments) {
		finishedSegments.Add(*f);
	}

	for (logicRoadSegment* s : allSegments){
		delete(s->segment);
		delete(s);
	}

	std::clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	UE_LOG(LogTemp, Warning, TEXT("time to generate road segments: %f"), elapsed_secs);


	return finishedSegments;
}

TArray<FMaterialPolygon> ASpawner::getRoadLines(TArray<FRoadSegment> segments)
{
	float lineInterval = 600;
	float lineLen = 350;
	float lineWidth = 40;
	TArray<FMaterialPolygon> lines;
	for (FRoadSegment f : segments) {
		if (f.type == RoadType::main) {
			FVector tangent = f.p2 - f.p1;
			tangent.Normalize();
			int spaces = FVector::Dist(f.p2, f.p1) / (lineInterval+1);
			for (int i = 1; i < spaces; i++) {
				FVector startPos = tangent * lineInterval * i + f.p1;
				FVector endPos = startPos + tangent*lineLen;
				FVector normal = getNormal(endPos, startPos, true);
				normal.Normalize();
				FMaterialPolygon line;
				line.type = PolygonType::roadMiddle;
				line.points.Add(startPos + normal*lineWidth);
				line.points.Add(startPos - normal*lineWidth);
				line.points.Add(endPos - normal*lineWidth);
				line.points.Add(endPos + normal*lineWidth);
				line.offset(FVector(0, 0, 15));
				lines.Add(line);
			}
		}
	}
	return lines;
}

TArray<FPolygon> ASpawner::roadsToPolygons(TArray<FRoadSegment> segments)
{
	TArray<FPolygon> polygons;
	for (FRoadSegment f : segments) {
		FPolygon p;
		p.points.Add(f.v1);
		p.points.Add(f.v3);
		p.points.Add(f.v4);
		p.points.Add(f.v2);
		polygons.Add(p);
	}
	return polygons;
}

TArray<FMaterialPolygon> ASpawner::roadPolygonsToMaterialPolygons(TArray<FPolygon> pols)
{
	TArray<FMaterialPolygon> polygons;
	for (int i = 0; i < pols.Num(); i++) {
		FPolygon p = pols[i];
		FMaterialPolygon fp;
		fp.type = PolygonType::asphalt;
		fp.points = p.points;
		fp.offset(FVector(0, 0, 10));
		polygons.Add(fp);
	}
	//for (FPolygon p : pols) {

	//}
	return polygons;
}






TArray<FTransform> ASpawner::visualizeNoise(int numSide, float noiseMultiplier, float posMultiplier) {
	TArray<FTransform> toReturn;
	for (int i = -numSide/2; i < numSide/2; i++) {
		for (int j = -numSide/2; j < numSide/2; j++) {
			FTransform f;
			f.SetLocation(FVector(posMultiplier * i, posMultiplier * j, 0));
			float res = NoiseSingleton::getInstance()->noise(f.GetLocation().X, f.GetLocation().Y);

			f.SetScale3D(FVector(posMultiplier/100, posMultiplier/100,  res* posMultiplier/10));
			toReturn.Add(f);
		}
	}
	return toReturn;
}




TArray<FMetaPolygon> ASpawner::getSurroundingPolygons(TArray<FRoadSegment> segments)
{
	return BaseLibrary::getSurroundingPolygons(segments, segments, standardWidth, extraLen, extraBlockingLen, 50, 100);
}

// Called when the game starts or when spawned
void ASpawner::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
