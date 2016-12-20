#!/bin/bash

rm *.o
../cleanup.sh
g++ -std=c++11 MyBot.cpp -o MyBot.o
#g++ -std=c++11 RandomBot.cpp -o RandomBot.o
./halite -d "30 30" "./MyBot.o" "../Halite-C++-Starter-Package/RandomBot.o"
