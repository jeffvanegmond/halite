#pragma once
#include "hlt.hpp"

#include <vector>

//bool hasEnemies(const hlt::Location& location, hlt::GameMap& map, bool include_map=true) {
//
//}


template<typename HeatCalculator>
class HeatMap {
	std::vector<std::vector<float>> heat;
	HeatMap() {}

	void generateHeat(HeatCalculator& calculator, hlt::GameMap& map) {
		for(unsigned short x = 0; x < map.width; ++x) {
			for(unsigned short y = 0; y < map.height; ++y) {
				hlt::Location loc = {x,y};
				heat[x][y] = calculator(loc, map);
			}
		}
	}
};

template <typename Heat>
class RegionHeatCalculator {
	unsigned short region;
	Heat h;
	RegionHeatCalculator(unsigned short region_size, Heat h) : region(region_size), h(h) {}

	float operator() (hlt::Location loc, hlt::GameMap& map) {
		float total_heat = 0;
		hlt::Location curr_loc;
		for(unsigned short i = 0; i < region; ++i) {
			curr_loc = map.getLocation(map.getLocation(loc, NORTH), WEST);
		}

		hlt::Location curr_loc_column = curr_loc;
		unsigned short neighborhood = region * 2 + 1;
		for(unsigned short i = 0; i < neighborhood; ++i) {
			for(unsigned short j = 0; j < neighborhood * 2 + 1; ++j) {
				total_heat += h(map, loc);
				curr_loc = map.getLocation(curr_loc, SOUTH);
			}
			curr_loc_column = map.getLocation(curr_loc_column, EAST);
			curr_loc = curr_loc_column;
		}
	}
};

template <typename Heat1, typename Heat2>
class CombinedHeatCalculator {
	Heat1 h1;
	Heat2 h2;
	float w1, w2;
	CombinedHeatCalculator(Heat1 h1, Heat2 h2) : h1(h1), h2(h2), w1(1.f), w2(1.f) {}
	CombinedHeatCalculator(Heat1 h1, Heat2 h2, float w1, float w2) : h1(h1), h2(h2), w1(w1), w2(w2) {}
	float operator() (hlt::GameMap& map, hlt::Location loc) {
		return w1 * h1(map, loc) + w2 * h2(map, loc);
	}
};

class ProductionHeatCalculator {
	float operator() (hlt::GameMap& map, hlt::Location loc) {
		return float(map.getSite(loc).production);
	}
};

class StrengthHeatCalculator {
	float operator() (hlt::GameMap& map, hlt::Location loc) {
		return float(map.getSite(loc).strength);
	}
};

class ProductionRatioHeatCalculator {
	float operator() (hlt::GameMap& map, hlt::Location loc) {
		hlt::Site s = map.getSite(loc);
		if(s.production == 0)
			return 0.f;
		if(s.strength == 0)
			return 255.f;
		return float(s.production) / float(s.strength);
	}
};

class DistanceHeatCalculator {
	hlt::Location cloc;
	DistanceHeatCalculator(hlt::Location loc) : cloc(loc) {}
	float operator() (hlt::GameMap& map, hlt::Location loc) {
		return float(map.getDistance(cloc, loc));
	}
};

template <typename Heat>
class OwnedHeatCalculator {
	unsigned char owner;
	float default_value;
	Heat h;
	OwnedHeatCalculator(unsigned char owner, Heat h) : owner(owner), h(h), default_value(0.f) {}
	OwnedHeatCalculator(unsigned char owner, Heat h, float def) : owner(owner), h(h), default_value(def) {}
	float operator() (hlt::GameMap& map, hlt::Location loc) {
		if(map.getSite(loc).owner == owner)
			return h(map, loc);
		return default_value;
	}
};
