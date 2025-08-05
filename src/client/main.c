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


/* PSEUDOCODE (client v0)

setup:
  server_address = 127.0.0.1 (default)
  door = 5000
  rate_ms = 500
  timeout_ms = 2000
  window = 10
  make a UDP socket
  seq = 0
  last10_rtts = empty list
  counts: sent=0, received=0, lost=0

helper: update_window_swing()
  if last10 has < 2 items: swing = 0
  else: swing = max(last10) - min(last10)

main loop (runs forever):
  wait until next tick (every rate_ms)
  seq += 1
  counts.sent += 1
  start_stopwatch = now()
  build text line: "PING seq=<seq> sent_at=<start_stopwatch>"
  send line to server_address:door

  wait up to timeout_ms for any reply
    if reply arrives:
      stop_stopwatch = now()
      rtt_ms = stop_stopwatch - start_stopwatch
      push rtt_ms into last10 (keep only last 10)
      counts.received += 1
      update_window_swing()
      print: "seq=<seq> rtt_ms=<rtt_ms> loss=0 window_swing_ms=<swing>"

    if no reply in time:
      counts.lost += 1
      // last10 doesn't change
      print: "seq=<seq> rtt_ms=NA loss=1 window_swing_ms=unchanged"
*/


// udp-monitor — minimal UDP echo server (v0)
// Listens on port 5000 and echoes back whatever it receives.

// udp-monitor — minimal UDP ping client (v0)
// Sends a ping every 500 ms, waits up to 2000 ms for a reply,
// measures RTT, keeps last-10 RTTs, and prints the swing.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <getopt.h>


int main(int argc, char *argv[]) {
    char *ADDR = "127.0.0.1";
    int PORT = 5000;
    int RATE_MS = 500;
    int TIMEOUT_MS = 2000;
    int WINDOW = 10;
    const size_t BUFSZ = 2048;


    static struct option long_opts[] = {
    {"address",    required_argument, 0, 'a'},
    {"door",       required_argument, 0, 'p'},
    {"rate-ms",    required_argument, 0, 'r'},
    {"timeout-ms", required_argument, 0, 't'},
    {"window",     required_argument, 0, 'w'},
    {"help",       no_argument,       0, 'h'},
    {0,0,0,0}
    };

    int opt, opt_index = 0;

    while ((opt = getopt_long(argc, argv, "a:p:r:t:w:h", long_opts, &opt_index)) != -1) {
        switch (opt) {
            case 'a': ADDR       = optarg;       break;
            case 'p': PORT       = atoi(optarg); break;
            case 'r': RATE_MS    = atoi(optarg); break;
            case 't': TIMEOUT_MS = atoi(optarg); break;
            case 'w': WINDOW     = atoi(optarg); break;
            case 'h':
            default:
                printf("Usage: %s [--address IP] [--door PORT] [--rate-ms MS] [--timeout-ms MS] [--window N]\n", argv[0]);
                return (opt=='h') ? 0 : 2;
        }
    }



    // 1) Create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    // 2) Set receive timeout
    struct timeval tv = { .tv_sec = TIMEOUT_MS/1000,
                          .tv_usec = (TIMEOUT_MS%1000)*1000 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // 3) Fill server address struct
    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(PORT);
    if (inet_pton(AF_INET, ADDR, &srv.sin_addr) != 1) {
        fprintf(stderr, "error: address '%s' not valid\n", ADDR);
        return 1;
    }

    // v1: client REGISTER with server
    pid_t my_pid = getpid();
    char regbuf[64];
    int rlen = snprintf(regbuf, sizeof(regbuf), "REGISTER pid=%d", my_pid);
    // send registration
    sendto(sock, regbuf, rlen, 0,
          (struct sockaddr *)&srv, sizeof(srv));


    // 4) Prep for loop
    int seq = 0, sent = 0, received = 0, lost = 0;
    double rtts[WINDOW];
    int rtt_count = 0;

    char sendbuf[BUFSZ], recvbuf[BUFSZ];

    // 5) Main loop
    while (1) {
        seq++; sent++;
        // timestamp start
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);

        // build ping message
        int len = snprintf(sendbuf, BUFSZ, "PING seq=%d", seq);
        sendto(sock, sendbuf, len, 0,
               (struct sockaddr *)&srv, sizeof(srv));

        // wait for pong
        ssize_t n = recvfrom(sock, recvbuf, BUFSZ, 0, NULL, NULL);
        if (n < 0) {
            lost++;
            printf("seq=%d rtt_ms=NA loss=%d window_swing_ms=unchanged\n",
                   seq, lost);
        } else {
            // timestamp end & compute RTT
            clock_gettime(CLOCK_MONOTONIC, &t1);
            double rtt = (t1.tv_sec - t0.tv_sec)*1e3
                       + (t1.tv_nsec - t0.tv_nsec)/1e6;

            // update rolling window
            if (rtt_count < WINDOW) {
                rtts[rtt_count++] = rtt;
            } else {
                memmove(rtts, rtts+1, (WINDOW-1)*sizeof(double));
                rtts[WINDOW-1] = rtt;
            }
            received++;

            // compute swing = max - min
            double mn = rtts[0], mx = rtts[0];
            for (int i = 1; i < rtt_count; i++) {
                if (rtts[i] < mn) mn = rtts[i];
                if (rtts[i] > mx) mx = rtts[i];
            }
            double swing = mx - mn;

            printf("seq=%d rtt_ms=%.1f loss=%d window_swing_ms=%.1f\n",
                   seq, rtt, lost, swing);
        }

        // wait RATE_MS before next ping
        struct timespec sleep_ts = {
            .tv_sec  = RATE_MS/1000,
            .tv_nsec = (RATE_MS%1000)*1000000
        };
        nanosleep(&sleep_ts, NULL);
    }

    close(sock);
    return 0;
}
