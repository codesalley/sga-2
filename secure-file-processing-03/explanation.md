# Q3 

### Command
```bash
./empdb
```

### How the open(), lseek(), write(), read(), and close() contributed

- **`open()` — creation, and the security boundary.** It returns the file descriptor every
  other call takes. Its subtler job is that creating the file and setting its permissions are
  *one atomic step*: `open(DBFILE, O_RDWR|O_CREAT|O_TRUNC, 0600)` means the file never exists
  in a world-readable state, not even briefly — the race that `creat()`-then-`chmod()` opens.
  The flags also encode intent, and the kernel enforces it: a descriptor opened `O_RDONLY`
  cannot be written through, no matter what the code does later.

- **`lseek()` — the call that makes the whole thing work.** It repositions the file offset
  without transferring any data, so `lseek(fd, n*RECSIZE, SEEK_SET)` reaches any record in one
  step. It is what turns "read record n" from a scan into a jump, and "update record n" from
  a whole-file rewrite into a 40-byte write. `lseek(fd, 0, SEEK_END)` doubles as a file-size
  query.

- **`write()` — persistence, sized to the change.** The size of the write is the point: 40
  bytes for one record, not 120 for the file. Writes land at the current offset and advance
  it, which is why the three initial records end up back to back with no seeking at all.

- **`read()` — retrieval.** Also advances the offset, which is exactly why the update path
  must seek *back* to offset 40 before writing: the `read()` left the offset at 80, and
  writing there would have overwritten Carol instead of Bob. That shared offset between
  `read` and `write` is the easiest thing to get wrong in this style of code.

- **`close()` — release, and a final error check.** Frees the descriptor. Its return value is
  checked because on some filesystems `close()` is where a deferred write error finally
  surfaces; ignoring it can turn a failed write into silent data loss.

