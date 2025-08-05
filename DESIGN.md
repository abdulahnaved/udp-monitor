1) Goal (short + sweet)
Build a tiny client/server that lives on my own laptop.
The client sends a tiny “ping” message to the server every so often.
The server immediately echoes it back.
The client times the round trip (there and back), keeps a short history, and prints easy-to-read health info.
Future versions will use these numbers to move clients between health “lanes” (Green → Yellow → Red).

2) Ground rules (so I don’t overthink)
Transport: UDP (postcard vibes: fast, might drop, that’s okay here).

Where it runs: both client and server on my own computer at first.

Door number (port): 5000 by default.

Keep it boring: text messages first (binary and fancy stuff can wait).

3) Vocabulary (plain words)
Address = which computer (v0: my own machine).

Port / Door = which door number on that computer (ex: 5000).

Ping-message = a tiny “you there?” postcard the client sends.

RTT (round-trip time) = time from send → reply back.

Jitter (simple) = how bouncy recent RTTs are (fastest vs slowest in the last 10).

4) Event flow (v0)
Client — every 500 ms
Increase a counter (seq).

Start a stopwatch.

Send a ping postcard to my own computer, door 5000.

Wait up to 2000 ms for a reply.

If reply arrives: stop the stopwatch → RTT = there-and-back time. Save it.

If no reply in time: mark this one as lost.

Keep only the last 10 RTT numbers (rolling list).

Print one line per ping:

Success: seq=42 rtt_ms=11.7 loss=0 window_swing_ms=1.2

Timeout: seq=43 rtt_ms=NA loss=1 window_swing_ms=unchanged

window_swing_ms = (fastest RTT − slowest RTT) in the last 10. This is my simple “jitter” signal for v0.

Server — always waiting
Wait at door 5000.

When any ping postcard arrives, send the same postcard right back to the sender. Immediately. That’s it for v0.

5) What data the client keeps (v0)
seq (starts at 1, +1 every ping)

sent / received / lost counts

last_10_rtts_ms (rolling list)

window_swing_ms (fastest minus slowest over that list)

6) Health rules (measured now, used later)
Lost postcard: no reply within 2000 ms.

Jittery: in the last 10, if (fastest to slowest) swing > 20 ms, that’s “bouncy”.

Slow: median RTT > 100 ms (once we have 10 values).

Decision rule (later): if any of these stays true for 3 checks in a row, the client will move to a worse lane; after moving, wait 10 s before moving again.

In v0, I only measure and print. Lane moves start in v1.

7) Knobs (defaults)
mode: server or client

address: my own computer (localhost)

door (port): 5000

rate_ms: 500 (send every 0.5 s)

timeout_ms: 2000

window: 10

8) Message shapes (keep it simple, text first)
Client → Server (ping): PING seq=<number> sent_at=<stopwatch_time>

Server → Client (pong): PONG seq=<same_number>

Notes:

“stopwatch_time” is just the client’s start time. The server doesn’t need to read it; it just sends the line back.

We’ll treat the whole line as a postcard. No parsing stress in v0 beyond “is it PONG and same seq?”.

9) What “done” looks like (acceptance)
With the server running and the client running:

I see small RTTs (a few ms) and loss = 0 most of the time.

The printed window_swing_ms is small and steady.

If I briefly stop the server (few seconds) then start it again:

The client logs loss = 1 during the gap, then goes back to normal.

After ~10–15 seconds I can point to:

sent / received / lost counts,

the last_10_rtts_ms,

and the current window_swing_ms.

10) Risks & gotchas (v0 realities)
On the same machine, RTTs will be tiny (that’s normal).

Wi-Fi can cause random bumps; that’s fine and actually helpful to see.

UDP is “postcard mail”: a message can get lost. That’s expected; the client should just log it and move on.

11) Future (short roadmap)
v1: add “lanes” (Green/Yellow/Red) and the 3-checks + 10s cooldown rule.

v2: nicer logs (JSON), config via flags/env/file, tiny diagram in README.

v3: multiple clients, server tracks stats per client address.

v4: automated tests + CI (unit math + simple integration runs).