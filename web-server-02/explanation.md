# Q2


### Command
```bash
./worker_pool
```
**Explanation:** Spawned 6 workers with a 5s timeout. Workers 0, 1, 3 and 4 finished on
their own and were reaped with `exited status 0`. At the 5s mark both stragglers were sent
`SIGTERM`; worker 5 died immediately (`killed by signal 15`), worker 2 ignored it and was
escalated to `SIGKILL` 2s later (`killed by signal 9`). The supervisor exited 0 only after
its `alive_count()` reached zero.

### Command
```bash
./worker_pool >/dev/null 2>&1 & SUP=$!
for i in 1 2 3 4 5 6 7 8; do
  echo "t+${i}s zombies=$(ps -o stat= -g $SUP | grep -c '^Z')"; sleep 1
done
```
**Explanation:** This is the direct evidence for the zombie requirement. It samples the
supervisor's process group once a second across the whole run and counts processes in state
`Z`. Result: `zombies=0` at all eight samples, including the seconds immediately after four
children exited at once. The children are reaped fast enough that a 1Hz sampler never
catches one in `Z` state.



### How creation, waiting and signalling combine

Each mechanism does one job, and none is sufficient alone:

- **`fork()` creates** the child and makes this process its *parent* — the link that gives us
  both the right to reap it and the PID to aim signals at. That PID and its fork timestamp go
  into `w[]`, which is what the timeout is later measured against.
- **`waitpid()` waits**, and is the only call that removes a zombie. `WNOHANG` makes it
  non-blocking, so one supervisor can watch N children instead of blocking on whichever
  happens to be first.
- **Signals carry the news, both ways.** *Inbound*, `SIGCHLD` says "someone died, go reap".
  *Outbound*, `SIGTERM`/`SIGKILL` are the only way to end a child that never exits on its own.

They form one cycle: `SIGCHLD` sets a flag → the loop reaps every dead child → any child past
its deadline gets signalled → the signal kills it → which raises `SIGCHLD` → which reaps it.

