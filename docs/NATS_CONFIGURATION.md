# NATS Configuration

Reference for the `NatsConfig` struct exposed by all three language wrappers (C#, C++, Python). It controls how the underlying [NATS.c](https://github.com/nats-io/nats.c) client connects, reconnects, and detects broken sockets.

The defaults shipped by libpitaya-cluster intentionally diverge from upstream NATS.c defaults — they are tuned for fast recovery from NATS pod evictions in Kubernetes (the dominant deployment shape across Wildlife). In most cases you should keep them.

## Where the defaults live

| Language | File | Notes |
| --- | --- | --- |
| C# | [`pitaya-sharp/NPitaya/Runtime/NativeInterop.cs`](../pitaya-sharp/NPitaya/Runtime/NativeInterop.cs) | Three constructor overloads + `CreateWithDefaults`; the simple `NatsConfig(string endpoint)` ctor seeds every field with the defaults below. |
| C++ | [`cpp-lib/include/pitaya/nats_config.h`](../cpp-lib/include/pitaya/nats_config.h) | `PITAYA_NATS_DEFAULT_*` macros + zero-arg constructor. |
| Python | [`python-lib/pitayaserver/c_interop.py`](../python-lib/pitayaserver/c_interop.py) | Bare ctypes struct — caller must set every field. |

The C++ wiring of each field into the NATS.c API lives in [`cpp-lib/src/pitaya/nats_client.cpp`](../cpp-lib/src/pitaya/nats_client.cpp). On startup the client logs every configured value at `info` level — useful when verifying that overrides reached the native layer.

## Parameter reference

All times are in milliseconds.

| Field | Default | NATS.c API | Purpose |
| --- | ---: | --- | --- |
| `endpoint` / `natsAddr` | — | `natsOptions_SetURL` | NATS server URL (e.g. `nats://localhost:4222`). |
| `connectionTimeoutMs` | 2000 | `natsOptions_SetTimeout` | How long the initial TCP/TLS handshake may take. |
| `requestTimeoutMs` | 2000 | (used by Pitaya RPC layer) | Per-RPC timeout for request/reply over NATS. |
| `serverShutdownDeadlineMs` | 2000 | (used during shutdown) | Max time to drain in-flight RPCs when this server shuts down. |
| `serverMaxNumberOfRpcs` | 500 | (used by RPC server) | Max concurrent in-flight inbound RPCs. |
| `maxPendingMessages` | 100 | `natsOptions_SetMaxPendingMsgs` | Async subscription queue depth (only set when the async API is used). |
| `reconnectBufSize` | `8 * 1024 * 1024` (8 MiB) | `natsOptions_SetReconnectBufSize` | Protocol-level buffer that holds outbound traffic while the connection is down. See [LAME_DUCK_MODE.md](../cpp-lib/docs/LAME_DUCK_MODE.md). |
| `maxReconnectionAttempts` | 30 | `natsOptions_SetMaxReconnect` + `natsOptions_SetRetryOnFailedConnect` | Reconnect attempt cap. Setting to `0` disables retry-on-failed-initial-connect. |
| `reconnectWaitInMs` | 100 | `natsOptions_SetReconnectWait` | Base delay between reconnect attempts. NATS.c default is 2000 ms. |
| `reconnectJitterInMs` | 50 | `natsOptions_SetReconnectJitter` | 0-N ms random delay added on top of `reconnectWaitInMs`. The same value is passed for both the plain and TLS jitter knobs. |
| `pingIntervalInMs` | 1000 | `natsOptions_SetPingInterval` | How often the client pings the server to detect a dead socket. NATS.c default is 120 000 ms. |
| `maxPingsOut` | 2 | `natsOptions_SetMaxPingsOut` | Unanswered pings tolerated before the connection is considered broken. Matches NATS.c default. |
| `drainTimeoutMs` | 1000 | (lame-duck path) | Drain deadline used during graceful shutdown. |
| `flushTimeoutMs` | 1000 | (lame-duck path) | Flush deadline used during graceful shutdown. |

## Reconnect & ping tuning

Five fields together govern how quickly a Pitaya server detects and recovers from a lost NATS connection — most often caused by a NATS pod being evicted, restarted, or rescheduled in Kubernetes. The shipped defaults are the platform team's recommendation; deviations should be deliberate.

### `reconnectWaitInMs` — 100 ms

Base delay before the next reconnect attempt. NATS.c's default of 2 s means that whenever a NATS pod dies, every client waits 2 s between tries — long enough for a noticeable burst of RPC failures (timeouts, broken pipes) to accumulate. 100 ms reconnects roughly 20× faster.

Trade-off: lower values mean tighter reconnect loops if NATS is genuinely down for a while. Combined with `maxReconnectionAttempts: 30`, the total attempt window is ~3-7 s (wait + jitter × 30) before the client gives up.

### `reconnectJitterInMs` — 50 ms

Random 0-50 ms delay added on top of `reconnectWaitInMs`. Without jitter, every client that lost the connection at the same moment retries at the same moment, which can produce a *reconnect storm* / *thundering herd* against the surviving pods. With jitter the retries spread out across a small window and recovery is smooth.

Keep this non-zero in any environment with more than a few clients. The exact value is not load-bearing — 50 ms is enough to break lockstep across hundreds of clients without meaningfully delaying recovery.

### `pingIntervalInMs` — 1000 ms + `maxPingsOut` — 2

How fast the client *notices* a broken socket when there is no traffic on it. NATS.c's default `pingInterval` of 2 minutes means a TCP-level outage can sit undetected until the next request fails — the application thinks the connection is fine and sends into a black hole. With these defaults a dead connection is detected in roughly `pingInterval × maxPingsOut` ≈ 2-3 s.

Trade-off: more wire chatter. With 1 s × 2 outstanding pings the cost is one tiny PING frame per second per connection — negligible for any realistic Pitaya server.

### `maxReconnectionAttempts` — 30

Hard cap on reconnect attempts before NATS.c reports the connection as permanently closed. Combined with the 100 ms + 50 ms jitter wait, this is roughly a 3-7 s recovery window.

NATS.c also uses this value to gate `RetryOnFailedConnect` (see [`nats_client.cpp:106-109`](../cpp-lib/src/pitaya/nats_client.cpp)): if it is `0`, the **initial** connection will fail immediately rather than retry, which can cause the server to refuse to start when NATS is briefly unavailable. Keep it `> 0` unless you specifically want fail-fast startup.

## Reconnect buffer

`reconnectBufSize` is the in-memory queue NATS.c uses to hold outbound messages while the connection is down. Defaults to 8 MiB. When the buffer fills up, new sends are rejected with `NATS_INSUFFICIENT_BUFFER` and the application must treat them as non-retryable.

The buffer interacts with hot-swap and request buffering during NATS server lame-duck mode. See [`cpp-lib/docs/LAME_DUCK_MODE.md`](../cpp-lib/docs/LAME_DUCK_MODE.md) for the full multi-level buffering architecture and per-environment sizing guidance (e.g. 64 MiB for high-throughput production, 8 MiB for dev).

## Lame-duck-mode parameters

`drainTimeoutMs`, `flushTimeoutMs`, and `serverShutdownDeadlineMs` are used when the local Pitaya server itself is shutting down or when a remote NATS server enters lame-duck mode. The shipped 1 s / 2 s defaults are appropriate for typical workloads. Increase them only if RPCs in your service routinely take longer than the default and you observe drops during rolling deploys. See [`cpp-lib/docs/LAME_DUCK_MODE.md`](../cpp-lib/docs/LAME_DUCK_MODE.md).

## Recommended C# configuration

For the common case, calling the simple constructor is enough:

```csharp
var natsConfig = new NatsConfig("nats://localhost:4222");
```

To make the configuration explicit (useful in code reviews and for partial overrides), pass every field by name:

```csharp
var natsConfig = new NatsConfig(
    endpoint: "nats://localhost:4222",
    connectionTimeoutMs: 2000,
    requestTimeoutMs: 2000,
    serverShutdownDeadlineMs: 2000,
    serverMaxNumberOfRpcs: 500,
    maxPendingMessages: 100,
    reconnectBufSize: 8 * 1024 * 1024,
    maxReconnectionAttempts: 30,
    reconnectWaitInMs: 100,
    reconnectJitterInMs: 50,
    pingIntervalInMs: 1000,
    maxPingsOut: 2);
```

Both produce identical effective configuration.

## References

- [NATS — Reconnect](https://docs.nats.io/using-nats/developer/connecting/reconnect)
- [NATS — Reconnect buffer](https://docs.nats.io/using-nats/developer/connecting/reconnect/buffer)
- [NATS — Lame duck mode](https://docs.nats.io/running-a-nats-service/nats_admin/lame_duck_mode)
- [`cpp-lib/docs/LAME_DUCK_MODE.md`](../cpp-lib/docs/LAME_DUCK_MODE.md) — in-tree reference for graceful shutdown, hot-swap, and the multi-level buffering architecture.
