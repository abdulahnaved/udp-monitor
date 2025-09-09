#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>

// Add JSON logging flag
int JSON_LOGGING = 0;

// ——— Lane definitions ———
#define LANE_GREEN   0
#define LANE_YELLOW  1
#define LANE_RED     2

// Map each lane to its UDP port
int lane_ports[] = {
    [LANE_GREEN]  = 5000,
    [LANE_YELLOW] = 6000,
    [LANE_RED]    = 7000
};

#define MAX_CLIENTS 16

typedef struct {
    pid_t pid;
    struct sockaddr_in addr;
    int current_lane;
    double rtts[10];
    int rtt_count;
    int loss_streak, slow_streak, jitter_streak;
    long cooldown_until_ms;
    int history_idx;
} client_t;

client_t clients[MAX_CLIENTS];
int client_count = 0;

long get_now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec*1000 + ts.tv_nsec/1000000;
}

void log_json(const char* level, const char* component, const char* event, const char* details) {
    if (JSON_LOGGING) {
        time_t now = time(NULL);
        printf("{\"timestamp\":\"%ld\",\"level\":\"%s\",\"component\":\"%s\",\"event\":\"%s\",\"details\":%s}\n", 
               now, level, component, event, details);
        fflush(stdout);
    }
}

void log_lane_switch(int pid, int old_lane, int new_lane, int new_port) {
    if (JSON_LOGGING) {
        printf("{\"timestamp\":\"%ld\",\"level\":\"INFO\",\"component\":\"server\",\"event\":\"lane_switch\",\"pid\":%d,\"old_lane\":%d,\"new_lane\":%d,\"new_port\":%d}\n",
               time(NULL), pid, old_lane, new_lane, new_port);
        fflush(stdout);
    }
}

void log_client_metrics(int pid, double rtt, int loss, double jitter, int loss_streak, int slow_streak, int jitter_streak, int lane) {
    if (JSON_LOGGING) {
        printf("{\"timestamp\":\"%ld\",\"level\":\"INFO\",\"component\":\"server\",\"event\":\"client_metrics\",\"pid\":%d,\"rtt\":%.2f,\"loss\":%d,\"jitter\":%.2f,\"loss_streak\":%d,\"slow_streak\":%d,\"jitter_streak\":%d,\"current_lane\":%d}\n",
               time(NULL), pid, rtt, loss, jitter, loss_streak, slow_streak, jitter_streak, lane);
        fflush(stdout);
    }
}



int main(int argc, char *argv[]) {
    int PORT = 5000;
    int VERBOSE = 0;
    const size_t BUFSZ = 2048;

    srand((unsigned)time(NULL));


    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--door") == 0 && i+1 < argc) {
            PORT = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--verbose") == 0) {
            VERBOSE = 1;
        }
        else if (strcmp(argv[i], "--json") == 0) {
            JSON_LOGGING = 1;
        }
    }
    if (JSON_LOGGING) {
        printf("{\"timestamp\":\"%ld\",\"level\":\"INFO\",\"component\":\"server\",\"event\":\"startup\",\"port\":%d,\"verbose\":%s}\n",
               time(NULL), PORT, VERBOSE ? "true" : "false");
        fflush(stdout);
    } else {
        printf("SERVER: listening on port %d%s\n",
               PORT, VERBOSE ? " (verbose)" : "");
    }


    // ——— Spawn clients on the Green lane ———
    int num_clients = 3;  
    for (int i = 0; i < num_clients; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child: exec the client binary on the Green port
            char port_arg[16];
            snprintf(port_arg, sizeof(port_arg), "%d", lane_ports[LANE_GREEN]);
            execl("./build/udp-monitor-client",
                  "udp-monitor-client",
                  "--door", port_arg,
                  (char*)NULL);
            perror("execl");
            exit(1);
        } else if (pid < 0) {
            perror("fork");
        }
    }


    // Create sockets for all three lanes
    int lane_fds[3];
    for (int i = 0; i < 3; i++) {
        lane_fds[i] = socket(AF_INET, SOCK_DGRAM, 0);
        if (lane_fds[i] < 0) {
            perror("socket");
            return 1;
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(lane_ports[i]);

        if (bind(lane_fds[i], (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("bind");
            close(lane_fds[i]);
            return 1;
        }
        if (JSON_LOGGING) {
            printf("{\"timestamp\":\"%ld\",\"level\":\"INFO\",\"component\":\"server\",\"event\":\"lane_ready\",\"port\":%d,\"lane\":%d}\n",
                   time(NULL), lane_ports[i], i);
            fflush(stdout);
        } else {
            printf("SERVER: listening on port %d (lane %d)\n", lane_ports[i], i);
        }
    }

    // lane_fds[LANE_GREEN] is used for sending control messages

    char buf[2048];

    srand((unsigned)time(NULL));

    for (;;) {
        // ─── 0) recv from any lane ─────────────────────────────────────────
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_fd = 0;
        for (int i = 0; i < 3; i++) {
            FD_SET(lane_fds[i], &read_fds);
            if (lane_fds[i] > max_fd) max_fd = lane_fds[i];
        }

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) { perror("select"); continue; }

        // Check which socket has data
        int active_fd = -1;
        for (int i = 0; i < 3; i++) {
            if (FD_ISSET(lane_fds[i], &read_fds)) {
                active_fd = lane_fds[i];
                break;
            }
        }
        if (active_fd == -1) continue;

        struct sockaddr_in peer;
        socklen_t peerlen = sizeof(peer);
        ssize_t n = recvfrom(active_fd, buf, BUFSZ, 0,
                             (struct sockaddr *)&peer, &peerlen);
        if (n < 0) { perror("recvfrom"); continue; }

        // ─── 1) METRIC handling (must be first!) ─────────────
        if (strncmp(buf, "METRIC", 6) == 0) {
            int pid, loss;
            double rtt, jitter;
            sscanf(buf,
                   "METRIC pid=%d rtt=%lf loss=%d jitter=%lf",
                   &pid, &rtt, &loss, &jitter);

            for (int i = 0; i < client_count; i++) {
                client_t *c = &clients[i];
                if (c->pid != pid) continue;

                // update rolling RTT window
                c->rtts[c->history_idx] = (rtt < 0 ? 0.0 : rtt);
                if (c->rtt_count < 10) c->rtt_count++;
                c->history_idx = (c->history_idx + 1) % 10;

                // streaks
                c->loss_streak = (loss > 0 ? c->loss_streak + 1 : 0);
                c->slow_streak = (rtt > 100.0 ? c->slow_streak + 1 : 0);
                // compute swing
                double mn = c->rtts[0], mx = c->rtts[0];
                for (int j = 1; j < c->rtt_count; j++) {
                    if (c->rtts[j] < mn) mn = c->rtts[j];
                    if (c->rtts[j] > mx) mx = c->rtts[j];
                }
                double swing = mx - mn;
                c->jitter_streak = (swing > 20.0 ? c->jitter_streak + 1 : 0);

                // decide new lane
                int triggers = (c->loss_streak >= 3)
                             + (c->slow_streak >= 3)
                             + (c->jitter_streak >= 3);
                int desired = triggers >= 2 ? LANE_RED
                              : triggers == 1 ? LANE_YELLOW
                                              : LANE_GREEN;

                // 🔍 debug-print and structured logging
                log_client_metrics(pid, rtt, loss, jitter, c->loss_streak, c->slow_streak, c->jitter_streak, c->current_lane);
                
                if (VERBOSE && !JSON_LOGGING) {
                    printf("DBG[%d]: pid=%d L/S/J=(%d/%d/%d) → trg=%d want=%d\n",
                           i, pid,
                           c->loss_streak,
                           c->slow_streak,
                           c->jitter_streak,
                           triggers,
                           desired);
                    fflush(stdout);
                }

                // send CONTROL if it’s time to switch
                long now = get_now_ms();
                if (desired != c->current_lane
                    && now >= c->cooldown_until_ms) {
                    int new_port = lane_ports[desired];
                    char ctrl[64];
                    int clen = snprintf(ctrl, sizeof(ctrl),
                                        "CONTROL pid=%d port=%d",
                                        pid, new_port);
                    sendto(lane_fds[LANE_GREEN], ctrl, clen, 0,
                           (struct sockaddr *)&c->addr,
                           sizeof(c->addr));
                    
                    int old_lane = c->current_lane;
                    c->current_lane      = desired;
                    c->cooldown_until_ms = now + 10000;
                    
                    log_lane_switch(pid, old_lane, desired, new_port);
                    
                    if (VERBOSE && !JSON_LOGGING) {
                        printf("SERVER: told pid=%d → lane%d(port=%d)\n",
                               pid, desired, new_port);
                        fflush(stdout);
                    }
                }
                break;
            }
            continue;  // done with this packet
        }

        // ─── 2) REGISTER handling ─────────────────────────────
        if (strncmp(buf, "REGISTER", 8) == 0) {
            int pid = atoi(strchr(buf, '=') + 1);
            if (client_count < MAX_CLIENTS) {
                client_t *c = &clients[client_count++];
                *c = (client_t){
                    .pid              = pid,
                    .addr             = peer,
                    .current_lane     = LANE_GREEN,
                    .rtt_count        = 0,
                    .history_idx      = 0,
                    .loss_streak      = 0,
                    .slow_streak      = 0,
                    .jitter_streak    = 0,
                    .cooldown_until_ms= 0
                };
                if (JSON_LOGGING) {
                    printf("{\"timestamp\":\"%ld\",\"level\":\"INFO\",\"component\":\"server\",\"event\":\"client_registered\",\"pid\":%d,\"client_index\":%d}\n",
                           time(NULL), pid, client_count-1);
                    fflush(stdout);
                } else {
                    printf("SERVER: registered pid=%d as client[%d]\n",
                           pid, client_count-1);
                }
                fflush(stdout);
            }
            continue;
        }

        // ─── 3) Chaos injection (only for PINGs) ─────────────

        if ( strncmp(buf, "PING", 4 ) == 0 )
        {
            if (rand() % 100 < 20) {
                if (JSON_LOGGING) {
                    printf("{\"timestamp\":\"%ld\",\"level\":\"DEBUG\",\"component\":\"server\",\"event\":\"chaos\",\"action\":\"drop\"}\n", time(NULL));
                    fflush(stdout);
                } else if (VERBOSE) {
                    printf("SERVER: dropping for chaos\n"); 
                    fflush(stdout);
                }
                continue;
            }
            int chaos_ms = rand() % 150;
            if (chaos_ms) {
                if (JSON_LOGGING) {
                    printf("{\"timestamp\":\"%ld\",\"level\":\"DEBUG\",\"component\":\"server\",\"event\":\"chaos\",\"action\":\"delay\",\"delay_ms\":%d}\n", time(NULL), chaos_ms);
                    fflush(stdout);
                } else if (VERBOSE) {
                    printf("SERVER: delaying %dms chaos\n", chaos_ms); 
                    fflush(stdout);
                }
                usleep(chaos_ms * 1000);
            }

            // ─── 4) Echo the PING ─────────────────────────────────
            ssize_t m = sendto(active_fd, buf, (size_t)n, 0,
                            (struct sockaddr *)&peer, peerlen);
            if (m < 0) { perror("sendto"); continue; }
        }
        // ─── 4) Verbose echo log ─────────────────────────────
        if (VERBOSE) {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
            if (JSON_LOGGING) {
                printf("{\"timestamp\":\"%ld\",\"level\":\"DEBUG\",\"component\":\"server\",\"event\":\"echo\",\"bytes\":%zd,\"client_ip\": \"%s\",\"client_port\":%d}\n",
                       time(NULL), n, ip, ntohs(peer.sin_port));
                fflush(stdout);
            } else {
                printf("echoed %zd bytes to %s:%d\n",
                       n, ip, ntohs(peer.sin_port));
                fflush(stdout);
            }
        }
    }

}
