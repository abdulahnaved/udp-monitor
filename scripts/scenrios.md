# Scenarios (later automation)

Scenario A — Happy path (localhost):
  - Start server on door 5000
  - Start client with defaults
  - Observe lines for ~10s

Scenario B — Timeout:
  - Kill server for a few seconds, then bring it back
  - Observe loss lines, then recovery

Scenario C — Jitter (later):
  - We'll use network emulation in a VM later (tc netem)
