#!/usr/bin/env bash
#
# gen_log.sh — fake application writing to a log, so the monitor has something
# to follow. Emits a mix of INFO/WARN/ERROR/FATAL, one line every 0.5s.
#
# usage: ./gen_log.sh [logfile] [num_lines]

set -u

LOG="${1:-app.log}"
N="${2:-20}"

levels=(INFO INFO INFO WARN ERROR INFO DEBUG ERROR INFO FATAL)
msgs=("request served" "cache miss" "db timeout" "disk 91% full"
      "connection refused" "user login" "retry scheduled" "null pointer")

for ((i = 1; i <= N; i++)); do
    lvl="${levels[$((RANDOM % ${#levels[@]}))]}"
    msg="${msgs[$((RANDOM % ${#msgs[@]}))]}"
    printf '%s %-5s [req-%04d] %s\n' "$(date '+%F %T')" "$lvl" "$i" "$msg" >>"$LOG"
    sleep 0.5
done
