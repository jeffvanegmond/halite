#include <stdlib.h>
#include <time.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <set>
#include <fstream>

#include "hlt.hpp"
#include "networking.hpp"
#include "helpers.hpp"

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
	h_log::cout << "*Considering square [x,y]=[" << int(location.x) << "," << int(location.y) << "]" << std::endl;
	unsigned char strategy = 0;
	bool on_border = hasEnemies(location, map);
	unsigned short turns_to_capture;
	unsigned short best_turns_to_capture = 255;
	unsigned char direction_to_capture = 0;
	if(on_border){
		float best_score = -255;
		int losses = 0;
		for(unsigned char direction = 1; direction < 5; ++direction) {
			hlt::Location destination_location = map.getLocation(location, direction);
			hlt::Site destination = map.getSite(destination_location);
			if(destination.owner == myID)
				continue;
			losses = destination.strength - map.getSite(location).strength;
			if(losses >= 0) {
				if(map.getSite(location).production == 0)
					continue;
				turns_to_capture = 1 + losses/map.getSite(location).production;
				if(turns_to_capture < best_turns_to_capture) {
					best_turns_to_capture = turns_to_capture;
					direction_to_capture = direction;
				}
				continue; // Note, due to overkill, it may be smart to actually attack a square we're not winning. Future investigation
			}
			for(unsigned char ok_direction = 1; ok_direction < 5; ++ok_direction) {
				hlt::Site surrounding = map.getSite(destination_location, ok_direction);
				if(surrounding.owner != myID && surrounding.owner != 0) {
					losses -= surrounding.strength;
				}
			}
			if(destination.production/losses > best_score) {
				best_score = destination.production/losses;
				strategy = direction;
			}
		}
		if( strategy != 0 ) {
			h_log::cout << "Capturing in direction " << int(strategy) << std::endl;
			return {location, strategy};
		}
	}

	h_log::cout << "Can't capture something. It will take me " << int(best_turns_to_capture) << " turns to capture " << int(direction_to_capture) << std::endl;

	if(!on_border && map.getSite(location).strength >= 5 * map.getSite(location).production) {
		return {location, findShortestPathToEnemy(location, map)};
	}

//	if(map.getSite(location).strength < 5 * map.getSite(location).production) {
//		return {location, strategy};
//	}

	h_log::cout << "Considering helping some other square" << std::endl;
	// See if I can aid a nearby square. If not, production.
	unsigned char help_direction = 0;
	unsigned char help_attack_dir = 0;
	unsigned short best_help_turns = 255;
	for(unsigned char dir = 1; dir < 5; ++dir) {
		hlt::Location aid_location = map.getLocation(location, dir);
		hlt::Site aid_site = map.getSite(aid_location);
		h_log::cout << "Let's see if I can help my neighbour in direction " << int(dir) << std::endl;
		if(aid_site.owner != myID) {
			h_log::cout << "Can't help that one, not mine." << std::endl;
			continue;
		}

		for(unsigned char attack_dir = 1; attack_dir < 5; ++attack_dir) {
			if(map.getSite(aid_location, attack_dir).owner == myID)
				continue;
			int defect = map.getSite(aid_location, attack_dir).strength - aid_site.strength;
			if(defect < 0)
				break; // No help required here, this unit can already capture

			if(aid_site.production == 0) {
				turns_to_capture = 255;
			}
			else {
				turns_to_capture = 1 + defect / aid_site.production;
			}
			
			int aided_defect = defect - map.getSite(location).strength;
			unsigned short aided_turns_to_capture;
			if(aided_defect < 0) {
				aided_turns_to_capture = 1;
			}
			else {
				if(aid_site.production == 0) {
					aided_turns_to_capture = 255;
				}
				else {
					aided_turns_to_capture = 1 + aided_defect / aid_site.production;
				}
			}

			if(aided_turns_to_capture < best_help_turns) {
				best_help_turns = aided_turns_to_capture;
				help_direction = dir;
				help_attack_dir = attack_dir;
			}
		}
	}

	int max_help_turns = 5;
	if(best_help_turns < best_turns_to_capture && best_help_turns <= max_help_turns) {
		h_log::cout << "I can help the square in direction " << int(help_direction) << " to capture a square in " << int(best_help_turns) << " turns." << std::endl;
		h_log::cout << "Let's see if they can also help me. We don't want to switch places and waste time, right?" << std::endl;
		hlt::Location help_location = map.getLocation(location, help_direction);
		hlt::Site help_site = map.getSite(help_location);
		int defect = map.getSite(location, direction_to_capture).strength - map.getSite(location).strength;
		defect -= help_site.strength;

		unsigned short helped_turns = 1;
		if(defect > 0) {
			helped_turns += defect / (map.getSite(location).production + help_site.strength);
		}
		if(helped_turns <= max_help_turns) {
			h_log::cout << "The site I want to help is likely going to help me, so let's stay put."  << std::endl;
			return {location, STILL};
		}
		h_log::cout << "Going to help!" << std::endl;
		return { location , help_direction };
	}

	return { location , STILL };
}


int main() {
	srand(time(NULL));

	
	std::cout.sync_with_stdio(0);

	unsigned char myID;
	hlt::GameMap presentMap;
	getInit(myID, presentMap);
	std::string filename = "game-";
	filename += std::to_string(int(myID)) + ".log";
	h_log::cout.open(filename, std::fstream::out);
	h_log::cout << " ===== STARTING GAME =====" << std::endl;
	sendInit("JeffExpBot v1");

	std::set<hlt::Move> moves;
	int turn = 0;
	while(true) {
		moves.clear();
		h_log::cout << " == Frame " << turn++ << std::endl;

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
