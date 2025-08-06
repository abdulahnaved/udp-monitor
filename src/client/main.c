// udp-monitor-client.c
// Sends PING → waits for PONG, reports RTT/loss/jitter
// Listens for CONTROL pid=<pid> port=<newPort> and switches lanes in place.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <getopt.h>
#include <sys/time.h>

// Add JSON logging flag
int JSON_LOGGING = 0;

int main(int argc, char *argv[]) {
    char *ADDR       = "127.0.0.1";
    int   PORT       = 5000;
    int   RATE_MS    = 1000;
    int   TIMEOUT_MS = 2000;
    int   WINDOW     = 10;
    const size_t BUFSZ = 2048;

    static struct option long_opts[] = {
        {"address",    required_argument, 0, 'a'},
        {"door",       required_argument, 0, 'p'},
        {"rate-ms",    required_argument, 0, 'r'},
        {"timeout-ms", required_argument, 0, 't'},
        {"window",     required_argument, 0, 'w'},
        {"json",       no_argument,       0, 'j'},
        {"help",       no_argument,       0, 'h'},
        {0,0,0,0}
    };

    int opt, opt_index = 0;
    while ((opt = getopt_long(argc, argv, "a:p:r:t:w:jh", long_opts, &opt_index)) != -1) {
        switch (opt) {
            case 'a': ADDR       = optarg;       break;
            case 'p': PORT       = atoi(optarg); break;
            case 'r': RATE_MS    = atoi(optarg); break;
            case 't': TIMEOUT_MS = atoi(optarg); break;
            case 'w': WINDOW     = atoi(optarg); break;
            case 'j': JSON_LOGGING = 1; break;
            case 'h':
            default:
                printf("Usage: %s [--address IP] [--door PORT] [--rate-ms MS] "
                       "[--timeout-ms MS] [--window N] [--json]\n", argv[0]);
                return (opt=='h') ? 0 : 2;
        }
    }

    // 1) Create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    // 2) Set receive timeout
    struct timeval tv = {
        .tv_sec  = TIMEOUT_MS / 1000,
        .tv_usec = (TIMEOUT_MS % 1000) * 1000
    };
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt SO_RCVTIMEO");
        close(sock);
        return 1;
    }

    // 3) Prepare server address
    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(PORT);
    if (inet_pton(AF_INET, ADDR, &srv.sin_addr) != 1) {
        fprintf(stderr, "error: invalid address '%s'\n", ADDR);
        close(sock);
        return 1;
    }

    // 4) REGISTER with server
    pid_t my_pid = getpid();
    char regbuf[64];
    int  rlen   = snprintf(regbuf, sizeof(regbuf), "REGISTER pid=%d", my_pid);
    if (sendto(sock, regbuf, rlen, 0,
               (struct sockaddr *)&srv, sizeof(srv)) < 0) {
        perror("sendto REGISTER");
        close(sock);
        return 1;
    }
    if (JSON_LOGGING) {
        printf("{\"timestamp\":\"%ld\",\"level\":\"INFO\",\"component\":\"client\",\"event\":\"registration\",\"pid\":%d,\"port\":%d}\n",
               time(NULL), my_pid, PORT);
        fflush(stdout);
    } else {
        printf("CLIENT: sent registration → %.*s\n", rlen, regbuf);
    }

    // 5) Prepare stats
    int    seq = 0, sent = 0, received = 0, lost = 0;
    double rtts[WINDOW];
    int    rtt_count  = 0;

    char sendbuf[BUFSZ], recvbuf[BUFSZ];

    while (1) {
        // ——— Ping round ———
        seq++; sent++;
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);

        int len = snprintf(sendbuf, BUFSZ, "PING seq=%d", seq);
        if (sendto(sock, sendbuf, len, 0,
                   (struct sockaddr *)&srv, sizeof(srv)) < 0) {
            perror("sendto PING");
            // we still attempt to recv CONTROL below
        }

        ssize_t n = recvfrom(sock, recvbuf, BUFSZ, 0, NULL, NULL);
        double rtt = -1;
        if (n < 0) {
            // timeout or error
            lost++;
            if (JSON_LOGGING) {
                printf("{\"timestamp\":\"%ld\",\"level\":\"INFO\",\"component\":\"client\",\"event\":\"ping_timeout\",\"seq\":%d,\"lost\":%d}\n",
                       time(NULL), seq, lost);
                fflush(stdout);
            } else {
                printf("seq=%d rtt_ms=NA loss=%d window_swing_ms=unchanged\n",
                       seq, lost);
            }
        } else {
            // Check if this is a CONTROL message first
            if (strncmp(recvbuf, "CONTROL", 7) == 0) {
                int recv_pid = 0, new_port = 0;
                char *p;
                if ((p = strstr(recvbuf, "pid=")))  recv_pid  = atoi(p+4);
                if ((p = strstr(recvbuf, "port="))) new_port  = atoi(p+5);

                if (recv_pid == my_pid && new_port > 0 && new_port != PORT) {
                    if (JSON_LOGGING) {
                        printf("{\"timestamp\":\"%ld\",\"level\":\"INFO\",\"component\":\"client\",\"event\":\"lane_switch\",\"pid\":%d,\"old_port\":%d,\"new_port\":%d}\n",
                               time(NULL), my_pid, PORT, new_port);
                        fflush(stdout);
                    } else {
                        printf("CLIENT: switching from port %d to %d\n",
                               PORT, new_port);
                        fflush(stdout);
                    }
                    PORT           = new_port;
                    srv.sin_port  = htons(new_port);
                }
                // Treat as lost packet since we expected a PONG
                lost++;
                if (JSON_LOGGING) {
                    printf("{\"timestamp\":\"%ld\",\"level\":\"INFO\",\"component\":\"client\",\"event\":\"ping_control\",\"seq\":%d,\"lost\":%d}\n",
                           time(NULL), seq, lost);
                    fflush(stdout);
                } else {
                    printf("seq=%d rtt_ms=NA loss=%d window_swing_ms=unchanged (got CONTROL)\n",
                           seq, lost);
                }
            } else {
                // Normal PONG processing
                clock_gettime(CLOCK_MONOTONIC, &t1);
                rtt = (t1.tv_sec - t0.tv_sec)*1e3 +
                      (t1.tv_nsec - t0.tv_nsec)/1e6;

                // rolling window
                if (rtt_count < WINDOW) {
                    rtts[rtt_count++] = rtt;
                } else {
                    memmove(rtts, rtts+1, (WINDOW-1)*sizeof(double));
                    rtts[WINDOW-1] = rtt;
                }
                received++;

                double mn = rtts[0], mx = rtts[0];
                for (int i = 1; i < rtt_count; i++) {
                    if (rtts[i] < mn) mn = rtts[i];
                    if (rtts[i] > mx) mx = rtts[i];
                }
                double swing = mx - mn;

                if (JSON_LOGGING) {
                    printf("{\"timestamp\":\"%ld\",\"level\":\"INFO\",\"component\":\"client\",\"event\":\"ping_success\",\"seq\":%d,\"rtt\":%.2f,\"lost\":%d,\"jitter\":%.2f,\"pid\":%d,\"port\":%d}\n",
                           time(NULL), seq, rtt, lost, swing, my_pid, PORT);
                    fflush(stdout);
                } else {
                    printf("seq=%d rtt_ms=%.1f loss=%d window_swing_ms=%.1f\n",
                           seq, rtt, lost, swing);
                }

                // send METRIC back to server over the same socket
                char metric[128];
                int  mlen = snprintf(metric, sizeof(metric),
                    "METRIC pid=%d rtt=%.1f loss=%d jitter=%.1f",
                    my_pid, rtt, (lost > 0 ? 1 : 0), swing);
                if (sendto(sock, metric, mlen, 0,
                           (struct sockaddr *)&srv, sizeof(srv)) < 0) {
                    perror("sendto METRIC");
                }
            }
        }

        struct timespec sleep_ts = {
            .tv_sec  = RATE_MS / 1000,
            .tv_nsec = (RATE_MS % 1000) * 1000000
        };
        nanosleep(&sleep_ts, NULL);
    }

    close(sock);
    return 0;
}
