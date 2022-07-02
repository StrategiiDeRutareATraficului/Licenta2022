
#include <math.h>
#include <iostream>
#pragma once
class KTreeRouter
{
private:

	int increment[6] = { 0 };
	int real_max;
	int max_level;

	int retrive_index(int value, int level)
	{
		return value / this->increment[level];
	}

public:

	int table_count[5][100000] = { 0 };

	KTreeRouter()
	{
		this->max_level = 5; 
		this->real_max = (int)pow(10, max_level);
		int inc = real_max / 10;
		for (int i = 0; i < max_level; i++)
		{
			increment[i] = inc;
			inc /= 10;
		}
	}

	void Update(int startTime, int endTime)
	{
		int indexStartTime;
		int indexEndTime;

		endTime++;

		for (int level = 0; level < max_level; level++)
		{
			indexStartTime = retrive_index(startTime, level);
			indexEndTime = retrive_index(endTime, level);

			table_count[level][indexStartTime]++;
			table_count[level][indexEndTime]--;
		}
	}

	long Query(int time)
	{
		long result = 0;
		int queryTime = 0;
		int start;
		int end;

		for (int level = 0; level < max_level - 1; level++)
		{
			start = retrive_index(queryTime, level);
			end = retrive_index(time, level);

			for (int index = start; index < end; index++)
			{
				queryTime += increment[level];
				result += table_count[level][index];
			}
		}

		start = retrive_index(queryTime, max_level - 1);
		end = retrive_index(time, max_level - 1);

		for (int index = start; index <= end; index++)
		{
			queryTime += increment[max_level - 1];
			result += table_count[max_level - 1][index];
		}

		return result;
	}

	// Calculam densitatea unui segment
	double edgeDensity(int time, double vehicleLenght, double segmentLenght, int numLanes)
	{
		// Interogam structura K-ary Single Point pentru a obtine numarul de masini de pe un segment la un moment de timp
		double numberOfCars = (double)Query(time);
		// Calculam densitatea
		double density = (numberOfCars * vehicleLenght) / (segmentLenght * numLanes);

		return density;
	}

	double computeEffort(int time, double vehicleLenght, double segmentLenght, double effort, int numLanes, bool caSumo, double baseDelay)
	{
		double density;
		double threshold[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 };
		int effortIncreaseConstant[] = { 1, 5, 10, 15, 20, 25, 30, 35, 40, 50 };

		if (caSumo)
		{
			density = edgeDensity(time, vehicleLenght, segmentLenght, numLanes);

			effort += baseDelay;

			// Crestem efortul in functie de densitate
			for (int i = 0; i < sizeof(threshold) / sizeof(threshold[0]); i++)
			{
				if ((density < threshold[i]) || (i == (sizeof(threshold) / sizeof(threshold[0]) - 1)))
				{
					return effort * effortIncreaseConstant[i];
				}

			}
		}
		else
		{
			return effort;
		}
	}

	double computeRealSpeed(int time, double vehicleLenght, double segmentLenght, double realSpeed, int numLanes) 
	{
		double density = edgeDensity(time, vehicleLenght, segmentLenght, numLanes);

		double threshold[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 };
		double speedDecreseConstant[] = { 1, 0.97, 0.87, 0.64, 0.42, 0.25, 0.17, 0.11, 0.07, 0.02 };

		// Scadem viteza maxima in functie de densitate
		for (int i = 0; i < sizeof(threshold) / sizeof(threshold[0]); i++)
		{
			if ((density < threshold[i]) || (i == (sizeof(threshold) / sizeof(threshold[0]) - 1)))
			{
				return realSpeed * speedDecreseConstant[i];
			}

		}

	}

	double computeDelay(int time, double vehicleLenght, double segmentLenght, double numLanes, double baseDelay)
	{
		double density = edgeDensity(time, vehicleLenght, segmentLenght, numLanes);

		double threshold[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 };
		int delayIncreaseConstant[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

		// Crestem timpul petrecut la semafore in functie de densitate
		for (int i = 0; i < sizeof(threshold) / sizeof(threshold[0]); i++)
		{
			if ((density < threshold[i]) || (i == (sizeof(threshold) / sizeof(threshold[0]) - 1)))
			{
				return baseDelay * delayIncreaseConstant[i];
			}

		}
	}
};

