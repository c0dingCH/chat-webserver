#!/bin/bash

CONCURRENCY=${1:-100}
TOTAL=${2:-10000}
HOST="127.0.0.1:8888"
URL="http://${HOST}/"

echo "=== Echo Server Benchmark ==="
echo "Concurrency: $CONCURRENCY | Total: $TOTAL"
echo ""

SUCCESS=0
FAIL=0
TOTAL_TIME=0

send_request() {
  local start=$(date +%s%N)
  local code
  code=$(echo "test" | nc -w 1 127.0.0.1 8888 2>/dev/null | wc -c)
  local end=$(date +%s%N)
  if [ $code -gt 0 ]; then
    SUCCESS=$((SUCCESS + 1))
    TOTAL_TIME=$((TOTAL_TIME + (end - start)))
  else
    FAIL=$((FAIL + 1))
  fi
}

export -f send_request
export HOST URL SUCCESS FAIL TOTAL_TIME

START=$(date +%s)

for i in $(seq 1 $TOTAL); do
  send_request &
  if [ $((i % CONCURRENCY)) -eq 0 ]; then
    wait
  fi
done
wait

END=$(date +%s)
ELAPSED=$((END - START))

if [ $SUCCESS -gt 0 ]; then
  AVG_US=$((TOTAL_TIME / SUCCESS))
  QPS=$(awk "BEGIN {printf \"%.2f\", $SUCCESS / $ELAPSED}")
else
  AVG_US=0
  QPS=0
fi

echo "Results:"
echo "  Total:    $TOTAL"
echo "  Success:  $SUCCESS"
echo "  Failed:   $FAIL"
echo "  Time:     ${ELAPSED}s"
echo "  QPS:      $QPS req/s"
echo "  Avg Lat:  ${AVG_US}us"