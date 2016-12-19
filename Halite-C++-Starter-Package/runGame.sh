#!/bin/bash

mkdir -p replays
mv *.hlt replays/
rm game.log
touch game.log
g++ -std=c++11 MyBot.cpp -o MyBot.o
g++ -std=c++11 RandomBot.cpp -o RandomBot.o
./halite -d "30 30" "./MyBot.o" "./RandomBot.o"
