/* microlink-lab — server v0 skeleton (no code yet)

GOAL (v0):
  - Wait at UDP "door" (port, default 5000).
  - For every line we receive, immediately send the same line back to the sender.

CLI knobs (later):
  --mode server
  --door 5000

Event flow:
  [bind UDP socket on door 5000]
  loop forever:
    - receive one datagram (text line)
    - send that same datagram back to the source address/door
    - (optional) print a tiny "echoed seq=X" line for visibility

Notes for later:
  - Keep messages as simple text lines.
  - Use non-fancy blocking I/O first (keep it boring).
  - Log errors with strerror(errno) when things fail.

TODOs to implement (in this order):
  [ ] Parse CLI args (only --mode, --door for server)
  [ ] Open UDP socket and bind to the chosen door
  [ ] recvfrom(...) into a small buffer
  [ ] sendto(...) with the same bytes back
  [ ] tiny printf per echo (optional)
*/
