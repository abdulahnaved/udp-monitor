# 🚀 UDP Monitor - Network Lane Switching System

A real-time UDP monitoring tool that demonstrates **network resilience** through automatic lane switching, having multiple network paths and automatically switching to the best one when problems occur.

## 🎯 What Does This Do?

Imagine you have 3 network connections:
- **🟢 Green Lane (Fast)** - Your primary connection
- **🟡 Yellow Lane (Medium)** - Your backup connection  
- **🔴 Red Lane (Slow)** - Your emergency connection

This system:
1. **Monitors network performance** (speed, packet loss, jitter)
2. **Automatically switches lanes** when performance degrades
3. **Logs everything** in structured JSON for analysis
4. **Simulates real problems** with chaos engineering

### Quick Start (WSL/Linux)
```bash
# Compile both server and client
wsl -d Ubuntu-24.04 gcc -O2 -Wall -Wextra -o build/udp-monitor-server src/server/main.c
wsl -d Ubuntu-24.04 gcc -O2 -Wall -Wextra -o build/udp-monitor-client src/client/main.c

# Run the complete test
./scripts/run-combined-tests.sh
```

### Cross-Platform with CMake
```bash
# For your current system
mkdir build && cd build
cmake ..
make

# For ARM (like Raspberry Pi)
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-linux-gnueabihf.cmake ..
make

# For other architectures
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/your-target.cmake ..
make
```

### 1. **Server Starts**
```
SERVER: listening on port 5000 (lane 0)
SERVER: listening on port 6000 (lane 1)  
SERVER: listening on port 7000 (lane 2)
```

### 2. **Server Automatically Creates 3 Client Children**
```
SERVER: registered pid=296 as client[0]
SERVER: registered pid=295 as client[1]
SERVER: registered pid=297 as client[2]
```

### 3. **Chaos Begins** (Simulating Network Problems)
```
SERVER: delaying 126ms chaos     ← Simulates slow network
SERVER: dropping for chaos       ← Simulates packet loss
```

### 4. **Lane Switching Kicks In**
```
SERVER: told pid=297 → lane1(port=6000)   ← Server decides to switch
CLIENT: switching from port 5000 to 6000  ← Client obeys and switches
```

## 🔄 The Parent-Child Architecture

**What makes this special:**

- **🏭 Server = Parent Process**: Manages everything
- **👶 Clients = Child Processes**: Automatically spawned by server
- **🔗 Parent-Child Communication**: Server monitors and controls children
- **🎯 Realistic Testing**: Additional manual clients can connect too

When you run the server, you get:
- **3 automatic children** (parent spawns via `fork()`)
- **Optional manual clients** (test scripts can add more)
- **Complete process management** (parent controls all children)

## 📊 Structured JSON Logging

Every event is logged as structured JSON:

### Lane Switch Event
```json
{
  "timestamp": "1754488323",
  "level": "INFO",
  "component": "server",
  "event": "lane_switch",
  "pid": 5320,
  "old_lane": 0,
  "new_lane": 1,
  "new_port": 6000
}
```

### Client Performance
```json
{
  "timestamp": "1754488322",
  "level": "INFO", 
  "component": "client",
  "event": "ping_success",
  "seq": 20,
  "rtt": 0.31,
  "lost": 3,
  "jitter": 144.55,
  "pid": 5320,
  "port": 7000
}
```


### Server
```bash
./build/udp-monitor-server --door 5000 --json --verbose
```
- `--door PORT`: Set primary port (default: 5000)
- `--json`: Output structured JSON logs
- `--verbose`: Show detailed debug information

### Client 
```bash
./build/udp-monitor-client --door 5000 --json
```
- `--door PORT`: Connect to server port
- `--json`: Output structured JSON logs



## 📁 Project Structure

```
udp-monitor/
├── src/
│   ├── server/main.c      # Main server with parent-child logic
│   └── client/main.c      # Client with lane switching
├── scripts/
│   ├── run-combined-tests.sh       # Complete test suite
│   └── logs/
├── cmake/                 # Cross-platform build configs
├── logs/                  # Generated log files
├── build/                 # Compiled binaries
└── test_lane_switching.sh # Interactive lane switching demo
```
