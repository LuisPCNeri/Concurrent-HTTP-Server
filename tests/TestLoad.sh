#!/bin/bash

# Pretty colors :)
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

URL="http://localhost:8080/"

tmp=$(mktemp)

FAIL=0
failed_requests=0

# 100k Requests for the server with 100 Concurrent
echo -e "${GREEN}Doing 100k requests with 100 concurrent...${NC}\n"

ab -q -n 100000 -c 100 "$URL" > "$tmp"
grep -E "Document Path:|Complete requests:|Failed requests:|Requests per second:|Time per request:" "$tmp" || cat "$tmp"

failed_requests=$(grep "Failed requests:" "$tmp" | awk '{print $3}')
FAIL=$(( "$FAIL" + "$failed_requests" ))

echo -e "\n"
rm -f "$tmp"

# Test multiple files
echo -e "${GREEN}Testing serving multiple diferent files${NC}\n"

tmp=$(mktemp)

for file in index.html style.css script.js; do
    ab -q -n 1000 -c 50 http://localhost:8080/$file > "$tmp"
    grep -E "Document Path:|Complete requests:|Failed requests:|Requests per second:|Time per request:" "$tmp" || cat "$tmp"

    # Update fail
    failed_requests=$(grep "Failed requests:" "$tmp" | awk '{print $3}')
    FAIL=$(( "$FAIL" + "$failed_requests" ))

    echo -e "\n"
done

rm -f "$tmp"

echo "===================================="

if [ "$FAIL" = 0 ]; then
    echo -e "${GREEN}ALL TESTS PASSED${NC}"
else
    echo -e "${RED}${FAIL} TESTS FAILED${NC}"
fi

echo "===================================="