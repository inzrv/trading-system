# Trading System

A minimal C++23 prototype of a Binance market data processing pipeline (L2 order book) with snapshot-based recovery.

The project consumes `depthUpdate` events from WebSocket, maintains a local order book, and attempts to recover via REST snapshot + buffered replay when a sequence gap is detected.
It also includes basic observability and optional raw market data recording for future replay experiments.

## Implemented So Far

- Binance WebSocket connection (`btcusdt@depth@100ms`)
- REST snapshot fetch (`/api/v3/depth?symbol=BTCUSDT&limit=5000`)
- JSON decoding into domain structures
- Sequence continuity checks (`Sequencer`)
- Update buffering during recovery
- Order book recovery via snapshot + replay
- Runtime metrics for counters and latency observations
- Optional market data recording into JSONL files:
  - WebSocket updates
  - REST snapshots with replay barrier metadata
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

## Configuration

The sample `config.json` enables Binance BTCUSDT market data and optional raw recording:

```json
"recording" : {
    "updatesPath" : "./data/updates.jsonl",
    "snapshotsPath" : "./data/snapshots.jsonl",
    "flushIntervalMs" : 250
}
```

Remove the `recording` section to run without market data recording.

## Data Flow

1. `Gateway` receives WS data and pushes raw payloads into a queue.
2. `Runtime` reads payloads from the queue.
3. `Decoder` parses JSON into `SequencedBookUpdate`.
4. `Sequencer` validates `U/u` continuity.
5. If the sequence is valid, `Orderbook` applies updates.
6. If a gap is detected, `RecoveryManager` performs snapshot + buffered replay.

## Market Data Recording

`MarketDataRecorder` writes raw input data to JSONL files in a background worker:

- `updates.jsonl` stores raw WebSocket update payloads with a monotonic `update_idx`.
- `snapshots.jsonl` stores raw REST snapshot payloads.
- Each snapshot also records `requested_after_update_idx` and `available_after_update_idx`.

Those two snapshot indexes are intended for deterministic replay:

- `requested_after_update_idx` is the last recorded WS update when the snapshot request was started.
- `available_after_update_idx` is the last recorded WS update when the snapshot response was recorded.
- A future replay gateway can use these barriers to avoid returning a snapshot too early relative to the replayed update stream.

## Current State and Limitations

- This is an infrastructure prototype, without strategy or execution logic.
- Market data can be recorded, but file-based replay is not implemented yet.
- Metrics exist, but there are no health checks or external metrics export yet.
- Supports a single source (Binance).

## Next Improvements

- Add integration tests for the decoder/sequencer/recovery pipeline.
- Add normalization into exchange-agnostic events.
- Implement file-based market data replay from recorded updates/snapshots.
- Reduce main-path latency overhead when market data recording is enabled.
