# AGENTS.md

## Project Overview
C++ chat server based on Linux networking. Architecture follows **Muduo-style Reactor pattern** (master-slave, event-driven).

Tech stack: C++17, epoll, Reactor, thread pool, HTTP/2.0, MySQL 8, Redis, WebSocket.

## Build & Run
```bash
cd Code
mkdir -p build && cd build
cmake .. && make
```
- Output: shared library `build/libchat.so`, test binaries in `build/test/`
- Compiler: `g++`, flags: `-g -Wall -Werror -std=c++17`

## Directory Structure
| Dir | Purpose |
|---|---|
| `base/` | Common utilities (CurrentThread, Latch) |
| `tcp/` | Reactor core: EventLoop, Channel, Poller, Acceptor, TcpConnection, TcpServer, Buffer |
| `http/` | HTTP server & protocol parsing |
| `https/` | HTTP/2.0 server (uses nghttp2) |
| `timer/` | TimerQueue (min-heap based) |
| `log/` | Async/sync logging system |
| `mysql/` | MySQL connection pool |
| `business/` | Business logic (User, Transport) |
| `test/` | Test binaries (echo, HTTP client/server, MySQL, timer, etc.) |
| `client_files/` | Client-side files for distribution |
| `public/` | Static HTTP serving root |
| `LogFiles/` | Log file output directory |

## Dependencies
- `pthread` (Threads)
- `mysqlclient`
- `nghttp2`

## Testing
Each `test/*.cpp` becomes a separate executable in `build/test/`. Run directly:
```bash
./build/test/<test_name>
```

## Key Conventions
- `file(GLOB ...)` collects sources per module — adding new `.cpp` files auto-includes them
- `-Werror` enabled — all warnings are errors
- Non-blocking IO + ET (edge-triggered) epoll
- Connection lifecycle uses `std::shared_ptr` + `enable_shared_from_this`
