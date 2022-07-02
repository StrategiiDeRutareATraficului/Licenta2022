
#include <math.h>
#include <iostream>
#pragma once
class K_Tree_Router
{
private:

	int table_count[3][500] = { 0 };
	int increment[3] = { 0 };
	int min_interval;
	int max_interval;
	int real_max;
	int max_level;
	long id;
	long lenght;
	double initial_speed;

	int retrive_index(int value, int level)
	{
		return value / this->increment[level];
	}

public:

	K_Tree_Router(long id, long lenght, int nr_of_nodes, double initial_speed)
	{
		this->id = id;
		this->lenght = lenght;
		this->initial_speed = initial_speed;
		this->min_interval = 0;
		this->max_interval = nr_of_nodes;
		this->max_level = (int)ceil(log10(nr_of_nodes));
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
		int queryTime = 1;
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

	double edgeDensity(int time, double vehicleLenght, double segmentLenght)
	{
		double numberOfCars = (double)Query(time);

		return (numberOfCars * vehicleLenght) / segmentLenght;
	}

	double computeSpeed(int time, double vehicleLenght, double segmentLenght)
	{
		// de verificat valoarea optima a lui penalizingCongestionValue
		double penalizingCongestionValue;
		return this->initial_speed * (1 - edgeDensity(time, vehicleLenght, segmentLenght) * penalizingCongestionValue);
	}
};

