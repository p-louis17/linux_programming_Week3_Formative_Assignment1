#!/bin/sh
# Starts the service in the background, sends every signal, and logs it all.
rm -f server_controller.pid demo_log.txt

send() {
    echo "--- $(date +%T) kill -$1 ---" >> demo_log.txt
    kill -"$1" "$PID"
}

echo "=== Run 1: SIGUSR1, SIGUSR2, SIGINT ===" >> demo_log.txt
./server_controller >> demo_log.txt 2>&1 &
PID=$!
sleep 1
send USR1; sleep 1
send USR2; sleep 1
send USR1; sleep 6          # wait for a tick: it should say MAINTENANCE
send USR2; sleep 1
send USR1; sleep 1
send INT
wait "$PID"; echo "exit status after SIGINT: $?" >> demo_log.txt
if ls server_controller.pid >/dev/null 2>&1; then
    echo "PID file after SIGINT: still exists" >> demo_log.txt
else
    echo "PID file after SIGINT: removed" >> demo_log.txt
fi

echo "" >> demo_log.txt
echo "=== Run 2: SIGTERM ===" >> demo_log.txt
./server_controller >> demo_log.txt 2>&1 &
PID=$!
sleep 7                      # one tick first
send TERM
wait "$PID"; echo "exit status after SIGTERM: $?" >> demo_log.txt
if ls server_controller.pid >/dev/null 2>&1; then
    echo "PID file after SIGTERM: still exists" >> demo_log.txt
else
    echo "PID file after SIGTERM: removed" >> demo_log.txt
fi
rm -f server_controller.pid