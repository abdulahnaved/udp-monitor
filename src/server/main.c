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


/* PSEUDOCODE (server v0)

setup:
  door = 5000 (default)
  make a UDP socket
  bind socket to door on my own computer

loop forever:
  wait to receive a message (a line of text)
  note who sent it (address + door)
  immediately send the same bytes back to that sender
  (optional) print: "echoed seq=<maybe parse later>"

errors:
  if receive/send fails, print an error line and keep going
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char *argv[]) {
    int PORT = 5000;
    int VERBOSE = 0;
    const size_t BUFSZ = 2048;


        // ——— manual flags ———
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--door") == 0 && i+1 < argc) {
            PORT = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--verbose") == 0) {
            VERBOSE = 1;
        }
        // ignore other args
    }
    printf("SERVER: listening on port %d%s\n",
           PORT, VERBOSE ? " (verbose)" : "");


    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in me;
    memset(&me, 0, sizeof(me));
    me.sin_family = AF_INET;
    me.sin_addr.s_addr = htonl(INADDR_ANY); // listen on all local addresses
    me.sin_port = htons(PORT);

    if (bind(fd, (struct sockaddr *)&me, sizeof(me)) < 0) {
        perror("bind");
        close(fd);
        return 1;
    }

    printf("udp-monitor-server listening on port %d\n", PORT);

    unsigned char buf[2048];
    for (;;) {
        struct sockaddr_in peer;
        socklen_t peerlen = sizeof(peer);
        ssize_t n = recvfrom(fd, buf, BUFSZ, 0, (struct sockaddr *)&peer, &peerlen);
        if (n < 0) {
            perror("recvfrom");
            continue; // keep running
        }

        // Echo the same bytes back to the sender
        ssize_t m = sendto(fd, buf, (size_t)n, 0, (struct sockaddr *)&peer, peerlen);
        if (m < 0) {
            perror("sendto");
            continue;
        }

        // Optional visibility:
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
        if (VERBOSE) {
          printf("echoed %zd bytes to %s:%d\n", n, ip, ntohs(peer.sin_port));
          fflush(stdout);
        }

    }
}
