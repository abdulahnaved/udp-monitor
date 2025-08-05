/* microlink-lab — client v0 skeleton (no code yet)

GOAL (v0):
  - Every 500 ms:
      * send a text "PING seq=<n> sent_at=<stopwatch>" to server@door
      * start a stopwatch at send time
      * wait up to 2000 ms for a reply "PONG seq=<n>"
      * on reply: RTT = stop - start; add to last-10 list; print a line
      * on timeout: mark lost; print a line; keep going

CLI knobs (later):
  --mode client
  --address 127.0.0.1 (my own computer for v0)
  --door 5000
  --rate_ms 500
  --timeout_ms 2000
  --window 10

Data we keep:
  - seq (starts at 1)
  - sent/received/lost counts
  - rolling array of last 10 RTTs (ms)
  - window_swing_ms = max(last10) - min(last10)

Print format (v0):
  - success:  "seq=42 rtt_ms=11.7 loss=0 window_swing_ms=1.2"
  - timeout:  "seq=43 rtt_ms=NA  loss=1 window_swing_ms=unchanged"

TODOs to implement (in this order):
  [ ] Parse CLI args (all the knobs above)
  [ ] Resolve server address + door
  [ ] Create UDP socket
  [ ] Main loop every rate_ms:
        - seq++
        - note start time (stopwatch)
        - send "PING ..." line
        - wait for reply up to timeout_ms
        - if reply with same seq: compute RTT, update last-10, print success line
        - else: count lost, print timeout line
*/
