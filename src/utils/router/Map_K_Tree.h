#include <math.h>
#include <iostream>
#include <router/RONet.h>
#include <unordered_map>
#include <utils/router/K_Tree_Router.h>
using namespace std;
#pragma once
class Map_K_Tree
{
private:
	std::unordered_map<string, K_Tree_Router> umap;
public:

	Map_K_Tree()
	{

	}

	void update(string id, int start, int end)
	{
		if (umap.find(id) != umap.end())
		{
			umap[id].Update(start, end);
		}
		else 
		{
			umap[id] = K_Tree_Router(1, 10, 500, 100);
			umap[id].Update(start, end);
		}
		
	}

	long query(string id, int time)
	{
		if (umap.find(id) != umap.end())
		{
			return umap[id].Query(time);
		}
		else
		{
			umap[id] = K_Tree_Router(1, 10, 500, 100);
			return umap[id].Query(time);
		}
		
	}
};