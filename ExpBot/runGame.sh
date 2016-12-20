#!/bin/bash

mkdir -p replays
mv *.hlt replays/
rm game.log
touch game.log
rm *.o
g++ -std=c++11 ExpBot.cpp -o ExpBot.o
#g++ -std=c++11 RandomBot.cpp -o RandomBot.o
./halite -d "30 30" "./MyBot.o" "./Halite-C++-Starter-Package/RandomBot.o"
