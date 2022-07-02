#include <math.h>
#include <iostream>
#include <unordered_map>
#include "KTreeRouter.h"
using namespace std;
#pragma once
class MapKTree
{
private:
	
public:
	// Folosind unordered_map atribuim fiecarui segment de drum o structura de tipul K-ary Single Point
	unordered_map<string, KTreeRouter*> umap;

	MapKTree()
	{

	}

	// Actualizare structura K-ary Single Point pentru segmentul procesat
	void update(string id, int start, int end)
	{
		// Verificam daca exista o structura K-ary Single Point pentru segmentul curent
		if (umap.find(id) != umap.end())
		{
			// Daca exista o actualizam
			umap[id]->Update(start, end);
		}
		else 
		{
			// Daca nu exista o cream
			umap[id] = new KTreeRouter(); 
			umap[id]->Update(start, end);
		}
		
	}

	// Verificam numarul de masinilor de pe un segment la un moment de timp 
	long query(string id, int time)
	{
		if (umap.find(id) != umap.end())
		{
			return umap[id]->Query(time);
		}
		else
		{
			return 0;
		}
		
	}

	// Calculam efortul pentru un segment
	double getEdgeEffort(int time, string segmentId, double vehicleLenght, double segmentLenght, 
						double effortDeltaOld, int numLanes, bool caSumo, double baseDelay)
	{
		if (umap.find(segmentId) != umap.end())
		{
			return umap[segmentId]->computeEffort(time, vehicleLenght, segmentLenght, effortDeltaOld, numLanes, caSumo, baseDelay);
		}
		else
		{
			umap[segmentId] = new KTreeRouter();
			return umap[segmentId]->computeEffort(time, vehicleLenght, segmentLenght, effortDeltaOld, numLanes, caSumo, baseDelay);
		}
		
	}

	// Estimam viteza maxima pentru un segment
	double getRealSpeed(int time, string segmentId, double segmentLenght, double vehiculeLenght, 
						double vehiculeSpeed, int numLanes)
	{
		if (umap.find(segmentId) != umap.end())
		{
			return umap[segmentId]->computeRealSpeed(time, vehiculeLenght, segmentLenght, vehiculeSpeed, numLanes);
		}
		else
		{
			umap[segmentId] = new KTreeRouter();
			return umap[segmentId]->computeRealSpeed(time, vehiculeLenght, segmentLenght, vehiculeSpeed, numLanes);
		}
	}
		
	// Calculam timpul petrecut la semafoare
	double getDelay(int time, string segmentId, double vehicleLenght, double segmentLenght,
					double numLanes, double baseDelay)
	{
		if (umap.find(segmentId) != umap.end())
		{
			return umap[segmentId]->computeDelay(time, vehicleLenght, segmentLenght, numLanes, baseDelay);
		}
		else
		{
			umap[segmentId] = new KTreeRouter();
			return umap[segmentId]->computeDelay(time, vehicleLenght, segmentLenght, numLanes, baseDelay);
		}
	}
};