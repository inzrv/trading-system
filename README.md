# Trading System

A minimal C++23 prototype of a Binance market data processing pipeline (L2 order book) with snapshot-based recovery.

The project consumes `depthUpdate` events from WebSocket, maintains a local order book, and attempts to recover via REST snapshot + buffered replay when a sequence gap is detected.

## Implemented So Far

- Binance WebSocket connection (`btcusdt@depth@100ms`)
- REST snapshot fetch (`/api/v3/depth?symbol=BTCUSDT&limit=5000`)
- JSON decoding into domain structures
- Sequence continuity checks (`Sequencer`)
- Update buffering during recovery
- Order book recovery via snapshot + replay
- Single-threaded core processing loop with a dedicated `boost::asio` I/O thread

## Tech Stack

- C++23
- CMake >= 3.20
- Boost (Asio/Beast/JSON)
- OpenSSL
- spdlog (git submodule)
- GoogleTest/GoogleMock (git submodule, optional for tests)

## Clone and Init Submodules

```bash
git clone <repo-url>
cd trading-system
git submodule update --init --recursive
```

## Build (Default: With Tests)

```bash
cmake -S . -B build
cmake --build build -j
```

## Build (Without Tests)

```bash
cmake -S . -B build-no-tests -DBUILD_TESTING=OFF
cmake --build build-no-tests -j
```

## Run Tests

Recommended:

```bash
ctest --test-dir build --output-on-failure
```

Direct run:

```bash
./build/tests/trading_system_tests
```

## Run Project With Config

```bash
./build/src/trading_system ./config.json
```

Or from the build without tests:

```bash
./build-no-tests/src/trading_system ./config.json
```

## Data Flow

1. `Gateway` receives WS data and pushes raw payloads into a queue.
2. `Runtime` reads payloads from the queue.
3. `Decoder` parses JSON into `SequencedBookUpdate`.
4. `Sequencer` validates `U/u` continuity.
5. If the sequence is valid, `Orderbook` applies updates.
6. If a gap is detected, `RecoveryManager` performs snapshot + buffered replay.


## Current State and Limitations

- This is an infrastructure prototype, without strategy or execution logic.
- No persistence/journal or file-based replay yet.
- No metrics, no health checks.
- Supports a single source (Binance).

## Next Improvements

- Add unit/integration tests for decoder/sequencer/recovery.
- Add normalization into exchange-agnostic events.
- Add journaling/replay and observability (metrics).
