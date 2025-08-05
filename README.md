microlink-lab (v0)
A tiny client/server app that lives on my own laptop.
The client sends a tiny “ping-message” to the server every so often.
The server immediately echoes it back.
The client times the round trip (there and back), keeps a short history, and prints simple health info.
Later versions will use these numbers to move clients between health “lanes” (Green → Yellow → Red).

Why this exists (match to the job)
Practice C on Linux and basic IP networking in a super small project.

Use UDP so I can see real timing (fast, may drop packets — that’s fine here).

Build in CMake, set up CI later, and add automated tests once the basics run.

Mental model (no jargon)
Address = which computer (for v0: my own computer).

Port = which door number on that computer (example: door 5000).

Server = waits at the door and replies to anyone who knocks.

Client = knocks on that door, then listens for the reply.

RTT (round-trip time) = how long it takes for a message to go there and back.

Jitter (simple) = how bouncy recent RTTs are (the swing between fastest and slowest).

Transport choice
Protocol: UDP (postcard vibes: quick, may get lost).

Why UDP: I care about timing signals (RTT + jitter) more than perfect delivery. TCP would hide some bumps.

v0 behavior (story)
Server waits at door 5000 (on my own computer).

Client every so often:

sends a ping-message (“yo, you there?”),

starts a stopwatch,

when the reply comes back, stops the stopwatch → that time is RTT,

saves it in a short history.

If a reply doesn’t come back in time, that ping is marked lost.

Defaults (locked)
Ping rate: every 500 ms

Timeout (give up and call it lost): 2000 ms

History window: 10 most recent pings

Loss trigger: more than 10% lost in the last 10 (so ≥1) for 3 checks in a row

Slow trigger: median RTT > 100 ms for 3 checks in a row

Jitter trigger (simple): (max RTT − min RTT) > 20 ms in the last 10 for 3 checks in a row

Cooldown after a move: 10 seconds

Lanes: Green (good), Yellow (warning), Red (bad)

(Note: v0 will measure and print. The lane-moving part comes right after v0.)

What the client prints (v0)
When it gets a reply:

ini
Copy
Edit
seq=42 rtt_ms=11.7 loss=0 window_swing_ms=1.2
When it times out:

ini
Copy
Edit
seq=43 rtt_ms=NA loss=1 window_swing_ms=unchanged
window_swing_ms = (max RTT − min RTT) over the last 10 pings.
This is my easy “jitter” signal for now.

Done-ness for v0
I can start a server that waits at a door on my own computer.

I can start a client that sends a ping-message every 500 ms.

I see small RTTs when the server is up, and loss = 0 most of the time.

If I stop the server briefly, the client shows loss = 1 for those moments, then recovers.

The client keeps a rolling list of the last 10 RTTs and shows the swing number.

Roadmap (short + realistic)
v0 (now): local client/server, RTT timing, history, simple prints.

v1: “lanes” logic (Green/Yellow/Red) with the triggers and cooldown above.

v2: nicer logs (JSON), config via flags/env/file, small diagrams in README.

v3: more clients, server tracking per-client stats, basic scheduling.

v4: tests + CI: unit tests for math, simple integration checks.

Non-goals (for now)
Encryption, auth, or internet-scale stuff.

Fancy binary protocols. (We’ll start with simple text messages to learn, can go binary later.)

Perfect measurement science. This is a learning project.