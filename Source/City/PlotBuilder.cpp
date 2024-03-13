// Fill out your copyright notice in the Description page of Project Settings.

#include "City.h"
#include "NoiseSingleton.h"
#include "PlotBuilder.h"
#include <random>
#include <ctime>


// Sets default values
APlotBuilder::APlotBuilder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void APlotBuilder::BeginPlay()
{
	Super::BeginPlay();
	
}

TArray<FMetaPolygon> APlotBuilder::sanityCheck(TArray<FMetaPolygon> plots, TArray<FPolygon> others) {
	TArray<FMetaPolygon> added;
	for (FMetaPolygon p : plots) {
		bool shouldAdd = !p.open;
		if (shouldAdd) {
			for (FMetaPolygon a : added) {
				if (testCollision(p, a, 0)) {
					shouldAdd = false;
					break;
				}
			}
		}
		if (shouldAdd)
			added.Add(p);
	}
	return added;
}
FLine getCrossingLine(float dist, FPolygon road) {
	FVector tanL = road.points[1] - road.points[0];
	float totLen = tanL.Size();
	tanL.Normalize();
	FVector startP = road.points[0] + totLen*dist*tanL;
	FVector endP = NearestPointOnLine(road.points[2], road.points[3] - road.points[2], startP);
	return{ startP, endP };
}


TArray<FMaterialPolygon> getCrossingAt(float dist, FPolygon road, float lineWidth) {
	FLine line = getCrossingLine(dist, road);
	if (line.p1.X == 0.0f) {
		UE_LOG(LogTemp, Warning, TEXT("Trouble with getCrossingAt() in PlotBuilder, returning zero vector"));
		return TArray<FMaterialPolygon>();
	}
	float lineInterval = 200;
	float lineLen = 100;
	TArray<FMaterialPolygon> lines;
	FVector tangent = line.p2 - line.p1;
	tangent.Normalize();
	int spaces = FVector::Dist(line.p1, line.p2) / (lineInterval + 1);
	for (int i = 1; i < spaces; i++) {
		FVector startPos = tangent * lineInterval * i + line.p1;
		FVector endPos = startPos + tangent*lineLen;
		FVector normal = getNormal(endPos, startPos, false);
		normal.Normalize();
		FMaterialPolygon newLine;
		newLine.type = PolygonType::roadMiddle;
		newLine.points.Add(startPos + normal*lineWidth);
		newLine.points.Add(endPos + normal*lineWidth);
		newLine.points.Add(endPos - normal*lineWidth);
		newLine.points.Add(startPos - normal*lineWidth);
		newLine.offset(FVector(0, 0, 11));
		lines.Add(newLine);
					
	}
	return lines;
}

FCityDecoration APlotBuilder::getCityDecoration(TArray<FMetaPolygon> plots, TArray<FPolygon> roads) {
	FCityDecoration dec;
	TMap<FMetaPolygon*, TSet<FMetaPolygon*>> connectionsMap;

	for (FPolygon road : roads) {
		FMetaPolygon *firstHit = nullptr;
		FMetaPolygon *sndHit = nullptr;
		FLine line = getCrossingLine(0.25, road);
		FLine testLine;
		FVector tan = line.p2 - line.p1;
		tan.Normalize();
		testLine.p1 = line.p1 - tan * 100;
		testLine.p2 = line.p2 + tan * 100;
		for (FMetaPolygon &plot : plots) {
			if (intersection(testLine.p1, testLine.p2, plot).X != 0.0f) {
				if (firstHit) {
					sndHit = &plot;
					// if these two plots werent previously connected, connections are added and crossing is placed, otherwise discard
					if (!connectionsMap[firstHit].Contains(sndHit)) {
						if (!connectionsMap.Contains(sndHit)) {
							connectionsMap.Add(sndHit, TSet<FMetaPolygon*>());
						}
						connectionsMap[firstHit].Add(sndHit);
						connectionsMap[sndHit].Add(firstHit);
						float width = 300;
						dec.polygons.Append(getCrossingAt(0.25, road, width));
						// add traffic lights
						FLine crossingLine = getCrossingLine(0.25, road);
						FRotator lookingDir = getNormal(crossingLine.p1, crossingLine.p2, true).Rotation();
						FVector offset = crossingLine.p2 - crossingLine.p1;
						offset.Normalize();
						offset *= -300;
						offset += lookingDir.RotateVector(FVector(700, 0, 0));
						if (FMath::FRandRange(0,0.9999) < 0.5) {
							dec.meshes.Add(FMeshInfo{ "traffic_light", FTransform{ lookingDir + FRotator(0,90,0), crossingLine.p1 - offset, FVector(1.0,1.0,1.0) } });
							dec.meshes.Add(FMeshInfo{ "traffic_light", FTransform{ lookingDir + FRotator(0,270,0), crossingLine.p2 + offset, FVector(1.0,1.0,1.0) } });
						}


						break;

					}
				}
				else {
					firstHit = &plot;
					if (!connectionsMap.Contains(firstHit)) {
						connectionsMap.Add(firstHit, TSet<FMetaPolygon*>());
					}
				}
			}
		}
	}

	return dec;
}

float getHeight(FRandomStream &stream, int minFloors, int maxFloors, FVector position, float noiseHeightInfluence) {
	float noise = NoiseSingleton::getInstance()->noise(position.X, position.Y);
	float adjustedNoiseFactor = (1.0 - noiseHeightInfluence) + (noise*noiseHeightInfluence);
	// value inbetween 0..1
	float modifier = -std::log(stream.FRandRange(std::min(1.02 - adjustedNoiseFactor/* e^(-4) */, 1.0), 1.0)) / 4;
	// value inbetween minFloors..maxFloors
	return minFloors + (maxFloors - minFloors)*modifier*adjustedNoiseFactor;
}

FHousePolygon getRandomModel(float minSize, float maxSize, int minFloors, int maxFloors, RoomType type, FRandomStream stream, float noiseHeightInfluence) {
	FHousePolygon pol;
	float xLen = stream.FRandRange(minSize, maxSize);
	float yLen = stream.FRandRange(minSize, maxSize);
	FVector tangent = FVector(stream.FRand(), stream.FRand(), 0);
	tangent.Normalize();
	FVector normal = FRotator(0, 90, 0).RotateVector(tangent);
	pol.points.Add(FVector(0, 0, 0));
	pol.points.Add(xLen * tangent);
	pol.points.Add(xLen * tangent + normal * yLen);
	pol.points.Add(normal*yLen);
	pol.points.Add(FVector(0, 0, 0));
	for (int i = 1; i < pol.points.Num(); i++) {
		pol.entrances.Add(i);
		pol.windows.Add(i);
		pol.open = false;
	}
	pol.housePosition = pol.getCenter();
	pol.height = getHeight(stream, minFloors, maxFloors, pol.getCenter(), noiseHeightInfluence);
	pol.type = type;
	pol.offset(-pol.getCenter());
	return pol;
}


FPlotInfo APlotBuilder::generateHousePolygons(FPlotPolygon p, int minFloors, int maxFloors) {
	FPlotInfo info;
	FVector cen = p.getCenter();
	FRandomStream stream(cen.X * 1000 + cen.Y);
	std::clock_t begin = clock();
	p.checkOrientation();
	float maxMaxArea = 6000.0f;
	float minMaxArea = 3000;
	float minArea = 1200.0f;

	UE_LOG( LogTemp, Warning, TEXT("size of p: %i, area: %f"), p.points.Num(), p.getArea());

	if (!p.open) {
		FHousePolygon original;
		original.points = p.points;
		original.checkOrientation();
		original.open = false;
		original.population = p.population;
		original.type = p.type;
		original.housePosition = original.getCenter();
		original.simplePlotType = p.simplePlotType;
		for (int32 i = 1; i < original.points.Num()+1; i++) {
			original.entrances.Add(i);
			original.windows.Add(i);
		}
		FVector center = p.getCenter();

		bool normalPlacement = p.getArea() < 3000 || stream.FRand() < 0.85;
		if (!normalPlacement) {
			// create a special plot with several similar houses placed around an area, this happens in real cities sometimes
			FHousePolygon model = getRandomModel(3500,6000, minFloors, maxFloors, p.type, stream, noiseHeightInfluence);
			model.checkOrientation();
			model.canBeModified = false;
			FPolygon shaft = AHouseBuilder::getShaftHolePolygon(model, stream);
			for (int i = 0; i < 3; i++) {
				TArray<FSimplePlot> temp;
				AHouseBuilder::makeInteresting(model, temp, shaft, stream);
			}

			TArray<FPolygon> placed;
			for (int i = 0; i < 6; i++) {
				FHousePolygon newH = model;
				newH.rotate(FRotator(0, stream.FRandRange(0, 360), 0));
				newH.offset(p.getRandomPoint(true, 2000));
				newH.housePosition = newH.getCenter();
				newH.type = p.type;
				newH.simplePlotType = p.simplePlotType;

				if (!testCollision(newH, placed, 0, p)) {
					info.houses.Add(newH);
					placed.Add(newH);
				}
			}
			if (placed.Num() < 1) {
				normalPlacement = true;
			}
			else {
				FSimplePlot fs = FSimplePlot(p.simplePlotType, p, simplePlotGroundOffset);
				fs.decorate(placed, instancedMap);
				info.leftovers.Add(fs);

			}
		}
		if (normalPlacement) {
			// have a chance of just making it empty
			if (stream.FRand() < 0.05) {
				FSimplePlot fs = FSimplePlot(p.simplePlotType, p, simplePlotGroundOffset);
				fs.decorate(instancedMap);
				info.leftovers.Add(fs);
			}
			else {
				float currMaxArea = stream.FRandRange(minMaxArea, maxMaxArea);
				if (p.getArea() > currMaxArea * 8) {
					// area is too large for even the max number of buildings, just make it a green simple plot
					FSimplePlot fs = FSimplePlot(SimplePlotType::green, p, simplePlotGroundOffset);
					fs.decorate(instancedMap);
					info.leftovers.Add(fs);
				}
				// too big to even be reasonable to make a simple plot, ignore it
				else if (p.getArea() > currMaxArea * 30) {
					// nothing
				}
				else {
				TArray<FHousePolygon> refinedPolygons = original.refine(currMaxArea, 0, 0);
				for (FHousePolygon r : refinedPolygons) {
					r.housePosition = r.getCenter();
					r.height = getHeight(stream, minFloors, maxFloors, r.getCenter(), noiseHeightInfluence);
					r.type = p.type;
					r.simplePlotType = p.simplePlotType;

					float area = r.getArea();

					if (area < minArea) {
						// too small, turn into simple plot
						FSimplePlot fs = FSimplePlot(p.simplePlotType, r, simplePlotGroundOffset);
						fs.type = p.simplePlotType;
						fs.decorate(instancedMap);
						info.leftovers.Add(fs);
					}
					else {
						info.houses.Add(r);
					}
				}
				}
			}

		}

	}
	std::clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	//FString res = FString();
	UE_LOG(LogTemp, Warning, TEXT("time to run generatehousepolygons: %f"), elapsed_secs);

	return info;

}


TArray<FMaterialPolygon> APlotBuilder::getSideWalkPolygons(FPlotPolygon p, float width) {
	TArray<FMaterialPolygon> pols;
	FVector prevP1 = FVector(0,0,0);
	FVector prevP2 = FVector(0, 0, 0);

	float endWidth = 40;
	float endHeight = 40;
	for (int i = 1; i < p.points.Num()+1; i++) {
		FVector p1 = p.points[i-1];
		FVector p2 = p.points[i%p.points.Num()];
		// add the straight part
		FMaterialPolygon current;
		current.type = PolygonType::concrete;
		FVector normal = getNormal(p1, p2, !p.isClockwise);
		normal.Normalize();
		current.points.Add(p1);
		current.points.Add(p2);
		current.points.Add(p2 + width*normal);
		current.points.Add(p1 + width*normal);
		current.offset(FVector(0, 0, 30));

		FMaterialPolygon currentOuterLine;
		currentOuterLine.type = PolygonType::concrete;
		currentOuterLine.points.Add(p2 + width*normal);
		currentOuterLine.points.Add(p2 + (width + endWidth)*normal);
		currentOuterLine.points.Add(p1 + (width+endWidth)*normal);
		currentOuterLine.points.Add(p1 + width*normal);

		currentOuterLine.offset(FVector(0, 0, endHeight));
		pols.Append(getSidesOfPolygon(currentOuterLine, PolygonType::concrete, endHeight));
		pols.Add(currentOuterLine);
		if (i != 1) {
			// add the corner
			FMaterialPolygon corner;
			corner.type = PolygonType::concrete;
			corner.points.Add(prevP1);
			corner.points.Add(p1 + width*normal);
			corner.points.Add(prevP2);
			//corner.points.Add(prevP1);
			corner.offset(FVector(0, 0, 30));

			FVector otherTan = prevP2 - prevP1;
			otherTan.Normalize();
			currentOuterLine.points.Empty();
			currentOuterLine.type = PolygonType::concrete;
			currentOuterLine.points.Add(prevP2);
			currentOuterLine.points.Add(p1 + width*normal);
			currentOuterLine.points.Add(p1 + (width+endWidth)*normal);
			currentOuterLine.points.Add(prevP2 + (endWidth)*otherTan);
			//currentOuterLine.points.Add(prevP2);
			currentOuterLine.offset(FVector(0, 0, endHeight));
			pols.Append(getSidesOfPolygon(currentOuterLine, PolygonType::concrete, endHeight));
			pols.Add(currentOuterLine);
			pols.Add(corner);
		}
		prevP1 = p2;
		prevP2 = p2 + width*normal;
		pols.Add(current);

	}

	FVector normal = getNormal(p.points[1], p.points[0], p.isClockwise);
	normal.Normalize();
	FMaterialPolygon corner;
	corner.type = PolygonType::concrete;
	corner.points.Add(p.points[0] + width*normal);
	corner.points.Add(prevP2);
	corner.points.Add(prevP1);

	FVector otherTan = prevP2 - prevP1;
	otherTan.Normalize();
	FMaterialPolygon currentOuterLine;
	currentOuterLine.type = PolygonType::concrete;
	currentOuterLine.points.Add(prevP2);
	currentOuterLine.points.Add(p.points[0] + width*normal);
	currentOuterLine.points.Add(p.points[0] + (width + endWidth)*normal);
	currentOuterLine.points.Add(prevP2 + (endWidth)*otherTan);
	currentOuterLine.offset(FVector(0, 0, endHeight));
	pols.Append(getSidesOfPolygon(currentOuterLine, PolygonType::concrete, endHeight));
	pols.Add(currentOuterLine);

	corner.offset(FVector(0, 0, 30));
	pols.Add(corner);
	return pols;
}

FPolygon APlotBuilder::generateSidewalkPolygon(FPlotPolygon p, float offsetSize) {
	FPolygon polygon;
	if (!p.open && p.getArea() > 700) {
		FVector center = p.getCenter();
		for (int i = 1; i < p.points.Num(); i++) {
			FVector tangent = p.points[i] - p.points[i - 1];
			tangent.Normalize();
			FVector offset = (p.isClockwise ? FRotator(0, 270, 0) : FRotator(0, 90, 0)).RotateVector(tangent * offsetSize);
			polygon.points.Add(p.points[i - 1] + offset);
			polygon.points.Add(p.points[i] + offset);
		}
		if (!p.open) {
			polygon.points.Add(FVector(polygon.points[1]));

		}
		else {
			FVector last = p.points[p.points.Num() - 1];
			polygon.points.Add(last);
		}
	}
	return polygon;
}

FSidewalkInfo APlotBuilder::getSideWalkInfo(FPolygon sidewalk)
{
	FSidewalkInfo toReturn;

	if (sidewalk.points.Num() < 2)
		return toReturn;
	// trees
	if (FMath::FRand() < 0.1f) {
		float placeRatio = 0.001;
		for (int i = 1; i < sidewalk.points.Num(); i += 2) {
			int toPlace = placeRatio * (sidewalk.points[i] - sidewalk.points[i - 1]).Size();
			for (int j = 1; j < toPlace; j++) {
				FVector origin = sidewalk.points[i - 1];
				FVector target = sidewalk.points[i];
				FVector tan = target - origin;
				float len = tan.Size();
				tan.Normalize();
				toReturn.meshes.Add(FMeshInfo{ "tree1", FTransform(origin + j * tan * (len / toPlace)) });
			}
		}
	}
	// lamp posts
	float placeRatio = 0.0007;

	for (int i = 1; i < sidewalk.points.Num(); i += 2) {
		int toPlace = placeRatio * (sidewalk.points[i] - sidewalk.points[i - 1]).Size();
		for (int j = 1; j < toPlace; j++) {
			FVector origin = sidewalk.points[i - 1];
			FVector target = sidewalk.points[i];
			FVector tan = target - origin;
			FVector normal = getNormal(origin, target, true);
			float len = tan.Size();
			tan.Normalize();
			toReturn.meshes.Add(FMeshInfo{ "lamppost", FTransform(normal.Rotation(), origin + j * tan * (len / toPlace)) });
		}
	}

	// fire hydrants
	float placeChance = 0.4;
	if (FMath::FRand() < placeChance) {
		int place = FMath::FRandRange(1, sidewalk.points.Num()-1);
		FVector rot = getNormal(sidewalk[place - 1], sidewalk[place], true);
		rot.Normalize();
		FVector loc = sidewalk[place - 1] + (sidewalk[place] - sidewalk.points[place - 1]) * FMath::FRand() + rot * 200 + FVector(0,0,15);
		toReturn.meshes.Add(FMeshInfo{ "fire_hydrant", FTransform(rot.Rotation(), loc) });

	}



	return toReturn;
}

TArray<FMaterialPolygon> APlotBuilder::getSimplePlotPolygonsPB(TArray<FSimplePlot> plots) {
	return BaseLibrary::getSimplePlotPolygons(plots);
}



// Called every frame
void APlotBuilder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

