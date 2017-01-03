#include <stdlib.h>
#include <time.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <set>
#include <fstream>

#include "astar.hpp"
#include "hlt.hpp"
#include "networking.hpp"

namespace h_log {
	std::fstream cout;
}

struct Data {
	unsigned short turn = 0;
	unsigned char myID;
	unsigned short neighborhood = 2;
	std::map<hlt::Location, float> interest;
	hlt::LocationQueue sorted_interest;
	hlt::LocationQueue my_locations;
};

void prepData(hlt::GameMap& map, Data& data) {
	data.interest.clear();
	data.sorted_interest = hlt::LocationQueue();
	data.my_locations = hlt::LocationQueue();

	h_log::cout << "=== NEW TURN(" << data.turn << "), PREPPING ===" << std::endl;
	++data.turn;

	for(unsigned short y = 0; y < map.height; y++) {
		for(unsigned short x = 0; x < map.width; x++) {
			hlt::Location l{x, y};

//			h_log::cout << "Evaluating: " << l << std::endl;

			if(map.getSite(l).owner == data.myID) {
				data.my_locations.push(std::make_pair(l, 0));
				continue;
			}

			// Scan neighborhood. First, go left and up some times
			hlt::Location curr_loc = l;
			for(unsigned short i = 0; i < data.neighborhood; ++i) {
				curr_loc = map.getLocation(map.getLocation(curr_loc, NORTH), WEST);
			}
//			h_log:: cout << "-- Moved to " << curr_loc << std::endl;

			// Then scan down and right
			hlt::Location curr_loc_column = curr_loc;
			int strength = 1; // Start this at 1 to prevent division by zero. It shouldn't affect the end result much.
			int production = 0;
			unsigned short region = data.neighborhood * 2 + 1;
			for(unsigned short i = 0; i < region; ++i) {
				for(unsigned short j = 0; j < region; ++j) {
					strength += map.getSite(curr_loc).strength;
					production += map.getSite(curr_loc).production;
					curr_loc = map.getLocation(curr_loc, SOUTH);
//					h_log::cout << "-- Evaluated all the way to: " << curr_loc << std::endl;;
				}
				curr_loc_column = map.getLocation(curr_loc_column, EAST);
				curr_loc = curr_loc_column;
			}

			data.interest[l] = float(production) / float(strength);
			data.sorted_interest.push(std::make_pair(l, data.interest[l]));
//			h_log::cout << "-- Interest was: " << data.interest[l] << ", composed of s=" << strength << " and p=" << production << std::endl;
//			h_log::cout << "-- Most interesting location so far: " << data.sorted_interest.top().first << " with i=" << data.sorted_interest.top().second << std::endl;
		}
	}

	// Go over my locations again
	hlt::LocationQueue new_priority;
	hlt::Location interesting = data.sorted_interest.top().first;
	while(!data.my_locations.empty()) {
		hlt::Location l = data.my_locations.top().first;
		data.my_locations.pop();
		float d = map.getDistance(l, interesting);
		new_priority.push(std::make_pair(l, 1.0/d));
	}
	data.my_locations = new_priority;
}

//bool hasEnemies(const hlt::Location& location, hlt::GameMap& map) {
//	unsigned char owner = map.getSite(location).owner;
//	for(unsigned char dir = 1; dir < 5; ++dir) {
//		if(map.getSite(location, dir).owner != owner)
//			return true;
//	}
//	return false;
//}
//
//unsigned char findShortestPathToEnemy(const hlt::Location& location, hlt::GameMap& map) {
//	int shortest_steps = std::max(map.height, map.width);
//	unsigned char strategy = 0;
//	unsigned char owner = map.getSite(location).owner;
//	float shortest_distance = float(shortest_steps);
//	float angle;
//	for(unsigned short x = 0; x < map.width; ++x) {
//		for(unsigned short y = 0; y < map.height; ++y) {
//			if(map.getSite({x, y}).owner == owner)
//				continue;
//			float distance = map.getDistance(location, {x, y});
//			if(distance < shortest_distance) {
//				shortest_distance = distance;
//				angle = map.getAngle(location, {x, y});
//			}
//		}
//	}
//	if(std::abs(angle) < M_PI / 4)
//		return EAST;
//	if(-angle < 3 * M_PI / 4 && -angle > M_PI / 4)
//		return NORTH;
//	if( angle < 3 * M_PI / 4 &&  angle > M_PI / 4)
//		return SOUTH;
//	return WEST;
//}

hlt::Move makeMove(const hlt::Location& location, hlt::GameMap& map, Data& data) {
//	unsigned char strategy = 0;
//	bool on_border = hasEnemies(location, map);
//	if(on_border){
//		float best_score = -255;
//		int losses = 0;
//		for(unsigned char direction = 1; direction < 5; ++direction) {
//			hlt::Site destination = map.getSite(location, direction);
//			losses = destination.strength - map.getSite(location).strength;
//			if(destination.owner != data.myID && losses < 0 && destination.production/losses > best_score) {
//				best_score = destination.production/losses;
//				strategy = direction;
//			}
//		}
//		if( strategy != 0 ) {
//			return {location, strategy};
//		}
//	}
//
//	if(map.getSite(location).strength < 5 * map.getSite(location).production) {
//		return {location, strategy};
//	}
//
//	if(!on_border) {
//		return {location, findShortestPathToEnemy(location, map)};
//	}
//
//	return { location , strategy };

	h_log::cout << ". Finding a path for " << location << std::endl;
	hlt::Location goal = data.sorted_interest.top().first;
	std::vector<unsigned char> path;
	hlt::MapDistance d{map};
	hlt::MapDistance h{map};
	hlt::findPath(location, goal, map, d, h, path, DEFAULT_INVALID);

	h_log::cout << ".. Found path is " << path.size() << " steps." << std::endl;
	hlt::Location step_location = map.getLocation(location, path[0]);
	h_log::cout << ".. The chosen step is " << int(path[0]) << " towards " << step_location << std::endl;
	hlt::Site step_site = map.getSite(step_location);
	hlt::Site loc_site = map.getSite(location);
	if(step_site.owner != data.myID && step_site.strength <= loc_site.strength) {
		h_log::cout << ".. My target is hostile and I'm taking it." << std::endl;
		return {location, path[0]};
	}

	if(step_site.owner == data.myID && loc_site.strength >= 5 * loc_site.production) {
		h_log::cout << ".. My target is friendly and I have some strength to move so I'm moving." << std::endl;
		return {location, path[0]};
	}


	h_log::cout << ".. I can't profitably make that move right now." << std::endl;
	return {location, 0};
}


int main() {
    srand(time(NULL));

	h_log::cout.open("game.log");
	h_log::cout << " ===== STARTING GAME =====" << std::endl;

    std::cout.sync_with_stdio(0);

    unsigned char myID;
    hlt::GameMap presentMap;
    getInit(myID, presentMap);

    Data data;
    data.myID = myID;

    sendInit("SmarterExpansionBot");

    std::set<hlt::Move> moves;
    while(true) {
        moves.clear();

        getFrame(presentMap);

        prepData(presentMap, data);
        hlt::LocationPriority interesting = data.sorted_interest.top();
        h_log::cout << "== Most interesting location:" << interesting.first << " with interest " << interesting.second << std::endl;

//        for(unsigned short a = 0; a < presentMap.height; a++) {
//            for(unsigned short b = 0; b < presentMap.width; b++) {
//                if (presentMap.getSite({ b, a }).owner == myID) {
//                    moves.insert(makeMove({ b, a },  presentMap, myID));
//                }
//            }
//        }
        while(!data.my_locations.empty()) {
            hlt::Location l = data.my_locations.top().first;
            moves.insert(makeMove(l, presentMap, data));
            data.my_locations.pop();
        }

        sendFrame(moves);
    }

	h_log::cout.close();
    return 0;
}
