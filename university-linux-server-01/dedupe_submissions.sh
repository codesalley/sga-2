#!/usr/bin/env bash


# stop execution if a non define variable is referenced
set -u   
# Throw and error if any command in the pipeline fails.                   
set -o pipefail            

SRC="${1:-./submissions}"
BACKUP="${2:-./backup}"
REPORT_DIR="./reports"
STAMP="$(date +%Y%m%d_%H%M%S)"

REPORT="$REPORT_DIR/report_$STAMP.txt"
ERRLOG="$REPORT_DIR/errors_$STAMP.log"
MANIFEST="$REPORT_DIR/manifest_$STAMP.tsv"
DUPES="$REPORT_DIR/duplicates_$STAMP.txt"

mkdir -p "$BACKUP" "$REPORT_DIR" || { echo "FATAL: cannot create working dirs" >&2; exit 1; }

# creating an empty file or truncating an existing file.
: > "$ERRLOG"


if command -v sha256sum >/dev/null 2>&1; then
    HASHER() { sha256sum "$1" | cut -d' ' -f1; }
elif command -v shasum >/dev/null 2>&1; then
    HASHER() { shasum -a 256 "$1" | cut -d' ' -f1; }
else
    echo "FATAL: no sha256sum or shasum on PATH" >>"$ERRLOG"
    exit 1
fi

if [ ! -d "$SRC" ]; then
    echo "FATAL: source directory '$SRC' does not exist" >>"$ERRLOG"
    exit 1
fi

processed=0
unique=0
duplicate=0
errors=0

# creating an empty file or truncating an existing file.
: > "$MANIFEST"
: > "$DUPES"


while IFS= read -r -d '' file; do
    processed=$((processed + 1))

    if ! sum="$(HASHER "$file" 2>>"$ERRLOG")"; then
        echo "ERROR: unreadable, skipped: $file" >>"$ERRLOG"
        errors=$((errors + 1))
        continue
    fi

    # grep -q exits 0 on first match: "have we already backed up this content?"
    if grep -q "^$sum	" "$MANIFEST"; then
        original="$(grep -m1 "^$sum	" "$MANIFEST" | cut -f2)"
        printf '%s\tDUPLICATE OF\t%s\n' "$file" "$original" >>"$DUPES"
        duplicate=$((duplicate + 1))
        continue
    fi

    rel="${file#"$SRC"/}"
    flat="${rel//\//__}"

    if cp -p "$file" "$BACKUP/$flat" 2>>"$ERRLOG"; then
        printf '%s\t%s\n' "$sum" "$file" >>"$MANIFEST"
        unique=$((unique + 1))
    else
        echo "ERROR: backup failed for: $file" >>"$ERRLOG"
        errors=$((errors + 1))
    fi
done < <(find "$SRC" -type f -print0 2>>"$ERRLOG")

{
    echo "==================================================="
    echo " SUBMISSION DE-DUPLICATION REPORT"
    echo " Generated : $(date '+%Y-%m-%d %H:%M:%S')"
    echo " Source    : $SRC"
    echo " Backup    : $BACKUP"
    echo "==================================================="
    printf ' Files processed   : %d\n' "$processed"
    printf ' Unique (backed up): %d\n' "$unique"
    printf ' Duplicates found  : %d\n' "$duplicate"
    printf ' Errors logged     : %d\n' "$errors"
    echo "---------------------------------------------------"
    if [ "$duplicate" -gt 0 ]; then
        echo " DUPLICATE SUBMISSIONS:"
        # Column-align the duplicate list for readability.
        sed 's/^/   /' "$DUPES"
    else
        echo " No duplicate submissions detected."
    fi
    echo "----------------------=----------------------------"
    echo " Full error log : $ERRLOG"
    echo " Hash manifest  : $MANIFEST"
    echo "=====================++==+========================="
} | tee "$REPORT"

[ "$errors" -eq 0 ]
