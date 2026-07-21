# Q4

### Command
```bash
chmod +x monitor_log.sh gen_log.sh
./monitor_log.sh app.log &  
./gen_log.sh app.log 20
```

### How pipes, grep, tail, redirection, and /dev/null improve efficiency.

- **Pipes (`|`)** are the reason this scales. Each stage is a separate process, and the kernel
  hands one stage's stdout directly to the next stage's stdin through an in-memory buffer —
  no temp file, no disk round-trip, no intermediate copy. All three stages run *concurrently*,
  so `grep` filters line *n* while `tail` is still waiting for line *n+1*. Memory stays flat
  regardless of log size: a 10 GB log costs the same as a 10 KB one, because no stage ever
  holds more than a line or two. The pipe also provides backpressure for free — if `tee` is
  slow, the pipe fills and `grep` blocks rather than accumulating an unbounded backlog.

- **`tail -n 0 -F`** turns a static file into a stream. `-F` follows *by name*, surviving
  rotation. `-n 0` starts at the end, so an already-large log is never re-read — startup cost
  is O(1) instead of O(file). Critically, `tail` blocks on the kernel until new data arrives
  rather than polling in a loop, so an idle monitor uses essentially no CPU.

- **`grep`** discards non-matching lines as early as possible — the single most valuable
  efficiency move in any pipeline, since every downstream stage then does less work. Here it
  cut 20 lines to 5, so `tee` performed 5 writes instead of 20. `-E` gives one alternation
  (`ERROR|FATAL`) instead of two passes, and `--line-buffered` trades a small amount of write
  efficiency for the latency the tool exists to provide.

- **Redirection (`>>`, `2>>`, `tee -a`)** separates the three output streams by *purpose*.
  `>>` appends rather than truncates, so a restart never destroys the existing report. `2>>`
  splits diagnostics away from data — the core requirement that the report contain log
  entries and nothing else. `tee -a` is what lets one stream reach two destinations without
  reading the source twice.

- **`/dev/null`** is a kernel-level discard: bytes written there are dropped without ever
  touching a disk or filling a buffer. It suppresses output that is *known* to be noise —
  permission warnings from directories the admin cannot read, or all output from a probe
  where only the exit status matters (`grep -q ... >/dev/null 2>&1`).