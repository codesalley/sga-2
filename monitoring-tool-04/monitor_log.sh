#!/usr/bin/env bash
#
# usage: ./monitor_log.sh [logfile] [reportfile]

set -u
set -o pipefail

LOG="${1:-app.log}"
REPORT="${2:-reports/errors.log}"
ERRLOG="reports/monitor_stderr.log"
PATTERN='ERROR|FATAL'

mkdir -p "$(dirname "$REPORT")" "$(dirname "$ERRLOG")"

if [ ! -e "$LOG" ]; then
    echo "FATAL: log file does not exist: $LOG" >&2
    exit 1
fi


printf '=== monitor started %s on %s ===\n' "$(date '+%F %T')" "$LOG" >>"$REPORT"

echo "monitoring $LOG for /$PATTERN/  (report: $REPORT, Ctrl-C to stop)"

trap 'echo; echo "stopped."; kill 0 2>/dev/null; exit 0' INT TERM

tail -n 0 -F "$LOG" 2>>"$ERRLOG" \
    | grep --line-buffered -E "$PATTERN" 2>>"$ERRLOG" \
    | tee -a "$REPORT"
