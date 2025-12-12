#!/bin/bash

#########################################################
## AUTHORS:
##  Luís Pedro Costa Néri Correia NMEC 125264
##  Guilherme Mendes Martins NMEC 125260
#########################################################

# Measure how cache impacts performance
# The file request will be a png as it is pretty big

# Pretty colors :)
GREEN='\033[0;32m'
NC='\033[0m'

echo -e "${GREEN}Requesting PNG file 5 times to make sure it is cached.${NC}"

for i in {1..5}; do
    # First requests to cache the item
    time curl -s http://localhost:8080/diamond.png > /dev/null
done

echo -e "${GREEN}Requesting PNG file 50 more times to see the difference the cache makes.${NC}"

for i in {1..50}; do
    time curl -s http://localhost:8080/diamond.png > /dev/null
done