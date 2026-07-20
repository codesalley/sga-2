# Q1

### File Structure
```sh
./backup    # submission files 
./submissions     # submission files 
/reports        # generates reports
```

### Command
```bash
chmod +x dedupe_submissions.sh && ./dedupe_submissions.sh
```
**Explanation:** `chmod +x` sets the execute bit so the kernel will honour the `#!/usr/bin/env bash`
shebang.


### Reproducing these results

`backup/` and `reports/` are regenerated on every run and are not checked in. Note that
`ivan_a1.py` ships readable, so a fresh run reports **9 processed / 5 unique / 4 duplicates
/ 0 errors** and exits 0. To reproduce the error-handling output shown above, first run:

```bash
chmod 000 submissions/ivan_a1.py
```

### Commands used

| Command | Why this one |
|---|---|
| `find -type f -print0` | Recurses the whole tree. `-type f` excludes directories and symlinks. `-print0` emits NUL-separated names — the only separator that cannot appear in a filename. |
| `sha256sum` / `shasum -a 256` | Content fingerprint. The script probes for both so it runs on GNU coreutils and BSD/macOS. |
| `cut -d' ' -f1` | Strips the filename off the checksum output, leaving the bare hash. |
| `grep -q` / `grep -m1` | `-q` exits on first match without printing — a cheap "have I seen this hash?" test. `-m1` stops after one match to fetch the original's path. |
| `cp -p` | Preserves mtime and mode, so submission timestamps survive into the archive — important if a late penalty is ever disputed. |
| `tee` | Writes the report to stdout *and* the report file in one pass, so the operator sees it live and it is still persisted. |
| `date +%Y%m%d_%H%M%S` | Timestamps each run's outputs so a re-run never overwrites yesterday's evidence. |

### Redirection operators

- `>` — truncate-and-write. Used via `: > "$MANIFEST"` to guarantee each run starts from an
  empty manifest. Without this, a stale manifest would make every file look like a duplicate.
- `>>` — append. Used for the manifest, duplicate list and error log, which are built up one
  line at a time inside the loop.
- `>/dev/null 2>&1` — on the `command -v` probes, where I only care about the exit status.
- `>&2` — sends the pre-log-file fatal message to stderr, since `$ERRLOG` isn't writable yet.

### File-handling techniques

**Process substitution instead of a pipe.** The loop reads `< <(find ...)`, not
`find ... | while`. In a pipeline bash runs the loop body in a *subshell*, so `processed`,
`unique` and `duplicate` would be incremented in a child process and revert to 0 the moment
the loop ended. Process substitution keeps the loop in the current shell, so the counters
survive. This is the single most common bug in scripts of this shape.

**NUL-delimited iteration.** `find -print0` paired with `read -r -d ''` handles filenames
containing spaces, quotes, or newlines. Student-supplied filenames are untrusted input, so
this is not hypothetical. `-r` also stops `read` from eating backslashes.

**Path flattening.** `${rel//\//__}` turns `late/heidi_a1.py` into `late__heidi_a1.py` before
copying into the flat backup directory. Without it, two students with the same basename in
different folders would silently overwrite each other in the archive.

**`set -u` and `set -o pipefail`.** `-u` aborts on an unset variable, catching typos rather
than acting on an empty string — `rm -rf "$TYPO/"` is the nightmare this prevents. `pipefail`
makes a pipeline fail if *any* stage fails; without it `sha256sum bad_file | cut ...` would
report success because `cut` succeeded, and an unreadable file would be miscounted as unique.

**Meaningful exit code.** The script ends with `[ "$errors" -eq 0 ]`, so it exits non-zero
when anything failed. Detect a bad run instead of assuming success.

