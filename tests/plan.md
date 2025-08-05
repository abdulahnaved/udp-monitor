# v0 test plan (manual for now)
- Normal path:
  1) Start server (door 5000).
  2) Start client (address 127.0.0.1, door 5000).
  3) Expect small RTTs, loss=0, and a small window_swing_ms.

- Timeout path:
  1) With client running, stop server for ~3 seconds.
  2) Expect client to print timeouts (loss=1 for those seq).
  3) Restart server; client recovers automatically.

- History/jitter:
  1) After ~10 pings, check the swing number is printed and updates.
