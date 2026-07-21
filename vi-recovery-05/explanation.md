# Q5


### Commands

```bash
ls -la .*.swp
```
**Explanation:** Check if there is a .swp file.

```bash
mkdir backup
cp .config.conf.swp ./backup 
cp config.conf ./backup 
```
**Explanation:** create a backup folder, copy .swp and cofig files into it as backup.

```bash
:w nginx.conf.recovered
```
**Explanation:** in **vim** vim write to __nginx.conf.recovered__ for comparison.

```bash
diff config.conf config.conf.recovered
```
**Explanation:** check the difference between the recovered.

```bash
mv config.conf.recovered config.conf
```
**Explanation:** replace the older config file with the newly recovered file.

```bash
rm .config.conf.swp 
```
**Explanation:** remove .swp file to prevent vi from reopening the recovery window when opening the file.

---

### Justification

Auto-recovery from the swap file is the most reliable mechanism for one structural reason: **the
swap file is the only artifact whose write is not triggered by a user action.** Undo files and
backups wait for `:w`; viminfo waits for a clean exit. A crash is precisely the event that
guarantees neither happens. The swap file is flushed on a timer during editing, so it holds state
that by definition has never been committed anywhere else — the exact state this scenario asks
about. Auto-recovery then makes that state usable without the developer having to know where the
swap file is, because vi checks for it automatically on every open.

---