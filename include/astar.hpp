#pragma once

#include <vector>
#include <queue>
#include <utility>
#include <map>

#include "hlt.hpp"

#define DEFAULT_INVALID 300.f

namespace hlt {

typedef std::pair<Location, float> LocationPriority;
typedef std::priority_queue<LocationPriority> LocationQueue;

bool operator<(const LocationPriority& a, const LocationPriority& b) {
	return a.second < b.second;
}

template<typename DistanceType, typename Heuristic>
void findPath(Location start, Location goal, GameMap& map, DistanceType& d, Heuristic& h, std::vector<unsigned char>& path, float ignore_threshold = DEFAULT_INVALID) {
	LocationQueue frontier;
	frontier.push(std::make_pair(start, 0.));
	std::map<Location, int> came_from;
	std::map<Location, float> cost_so_far;
	came_from[start] = 0;
	cost_so_far[start] = 0;

	LocationPriority current;
	Locations neighbors;
	while(!frontier.empty()) {
		current = frontier.top();
		frontier.pop();

		if(current.first == goal) {
			break;
		}

		getNeighbors(current.first, map, neighbors);
		Location next;
		for(int next_dir : CARDINALS) {
			next = map.getLocation(current.first, next_dir);
			LocationPriority lp = std::make_pair(next, 0.);
			float dist = d(current.first, next);
			if(dist > ignore_threshold)
				continue;
			lp.second = cost_so_far[current.first] + dist;

			if(cost_so_far.count(next) == 0 || cost_so_far[next] > lp.second) {
				cost_so_far[next] = lp.second;
				lp.second += h(goal, next);
				lp.second *= -1.;
				frontier.push(lp);
				came_from[next] = oppositeCardinal(next_dir);
			}
		}
	}
	
	path.clear();
	if(came_from.count(goal) == 0) {
		return;
	}

	Location curr = goal;
	int from_dir = came_from[goal];
	while(curr != start) {
		path.push_back(oppositeCardinal(from_dir));
		curr = map.getLocation(curr, from_dir);
		from_dir = came_from[curr];
	}
}

struct MapDistance {
	GameMap* map;
	MapDistance(GameMap& map) : map(&map) {}
	float operator()(Location& from, Location& to) {
		return map->getDistance(from, to);
	}
};

struct OwnedMapDistance {
	GameMap* map;
	float invalid_value;
	OwnedMapDistance(GameMap& map, float invalid_value = DEFAULT_INVALID) : map(&map), invalid_value(invalid_value) {}
	float operator()(Location& from, Location& to) {
		return map->getSite(from).owner == map->getSite(to).owner ?
		       map->getDistance(from, to) :
		       invalid_value;
	}
};

struct EuclideanDistance {
	GameMap* map;
	EuclideanDistance(GameMap& map) : map(&map) {}
	float operator()(Location& from, Location& to) {
		return map->getEuclideanDistance(from, to);
	}
};

struct StrengthCostDistance {
	GameMap* map;
	StrengthCostDistance(GameMap& map) : map(&map) {}
	float operator()(Location& from, Location& to) {
		if(map->getSite(to).owner == map->getSite(from).owner)
			return map->getSite(from).production;
		return map->getSite(to).strength;
	}
};

struct ZeroDistance {
	float operator()(Location& from, Location& to) {
		return 0;
	}
};

}
