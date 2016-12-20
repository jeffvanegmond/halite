#include <stdlib.h>
#include <time.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <set>
#include <fstream>

#include "hlt.hpp"
#include "networking.hpp"

namespace h_log {
	std::fstream cout;
}

bool hasEnemies(const hlt::Location& location, hlt::GameMap& map) {
	unsigned char owner = map.getSite(location).owner;
	for(unsigned char dir = 1; dir < 5; ++dir) {
		if(map.getSite(location, dir).owner != owner)
			return true;
	}
	return false;
}

unsigned char findShortestPathToEnemy(const hlt::Location& location, hlt::GameMap& map) {
	int shortest_steps = std::max(map.height, map.width);
	unsigned char strategy = 0;
	unsigned char owner = map.getSite(location).owner;
	float shortest_distance = float(shortest_steps);
	float angle;
	for(unsigned short x = 0; x < map.width; ++x) {
		for(unsigned short y = 0; y < map.height; ++y) {
			if(map.getSite({x, y}).owner == owner)
				continue;
			float distance = map.getDistance(location, {x, y});
			if(distance < shortest_distance) {
				shortest_distance = distance;
				angle = map.getAngle(location, {x, y});
			}
		}
	}
	if(std::abs(angle) < M_PI / 4)
		return EAST;
	if(-angle < 3 * M_PI / 4 && -angle > M_PI / 4)
		return NORTH;
	if( angle < 3 * M_PI / 4 &&  angle > M_PI / 4)
		return SOUTH;
	return WEST;
}

hlt::Move makeMove(const hlt::Location& location, hlt::GameMap& map, unsigned char myID) {
	unsigned char strategy = 0;
	bool on_border = hasEnemies(location, map);
	if(on_border){
		int best_score = 0;
		for(unsigned char direction = 1; direction < 5; ++direction) {
			hlt::Site destination = map.getSite(location, direction);
			if(destination.owner != myID && destination.strength - map.getSite(location).strength < best_score) {
				best_score = destination.strength - map.getSite(location).strength;
				strategy = direction;
			}
		}
		if( strategy != 0 ) {
			return {location, strategy};
		}
	}

	if(map.getSite(location).strength < 5 * map.getSite(location).production) {
		return {location, strategy};
	}

	if(!on_border) {
		return {location, findShortestPathToEnemy(location, map)};
	}

	return { location , strategy };
}


int main() {
    srand(time(NULL));

	h_log::cout.open("game.log");
	h_log::cout << " ===== STARTING GAME =====" << std::endl;

    std::cout.sync_with_stdio(0);

    unsigned char myID;
    hlt::GameMap presentMap;
    getInit(myID, presentMap);
    sendInit("SimpleExpansionBot");

    std::set<hlt::Move> moves;
    while(true) {
        moves.clear();

        getFrame(presentMap);

        for(unsigned short a = 0; a < presentMap.height; a++) {
            for(unsigned short b = 0; b < presentMap.width; b++) {
                if (presentMap.getSite({ b, a }).owner == myID) {
                    moves.insert(makeMove({ b, a },  presentMap, myID));
                }
            }
        }

        sendFrame(moves);
    }

	h_log::cout.close();
    return 0;
}
