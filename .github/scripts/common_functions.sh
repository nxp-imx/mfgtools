#!/bin/bash

log_and_run() {
    echo "[INFO] $1"
    shift
    "$@"
}

dump_logs() {
    echo "=== DUMPING ALL AVAILABLE LOGS ==="
    
    # vcpkg logs
    if [ -d "/tmp/vcpkg/buildtrees" ]; then
        find /tmp/vcpkg/buildtrees -name "*.log" -print0 | while IFS= read -r -d '' logfile; do
            echo "=== LOG: $logfile ==="
            cat "$logfile" 2>/dev/null || echo "Could not read $logfile"
            echo "=== END LOG ==="
        done
    fi
    
    # Project build logs
    if [ -d "build" ]; then
        find build -name "*.log" -print0 | while IFS= read -r -d '' logfile; do
            echo "=== LOG: $logfile ==="
            cat "$logfile" 2>/dev/null || echo "Could not read $logfile"
            echo "=== END LOG ==="
        done
    fi
}
