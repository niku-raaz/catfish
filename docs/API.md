# Local Catfish API

The Express bridge binds to `127.0.0.1:8787` by default. Development Vite
proxies `/api` to it.

## `GET /api/health`

Response:

```json
{
  "status": "ready",
  "engine": "Catfish"
}
```

`status` is `ready`, `starting`, or `failed`. A failed response uses HTTP 503
and may include a `detail` string.

## `POST /api/search`

Request:

```json
{
  "fen": "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
  "depth": 3
}
```

The schema is strict:

- `fen` must be 15–120 characters and pass server-side `chess.js` validation.
- `depth` must be an integer from 1 through 5.
- Unknown fields are rejected.

Response:

```json
{
  "bestMove": "e2e4",
  "info": {
    "depth": 3,
    "selectiveDepth": 6,
    "score": { "type": "cp", "value": 18 },
    "nodes": 1287,
    "quiescenceNodes": 814,
    "hashHits": 42,
    "tablebaseHits": 0,
    "source": "search",
    "pv": ["e2e4", "e7e5", "g1f3"]
  },
  "fen": "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
}
```

`bestMove` is null for a terminal position. `score.type` is `cp` or `mate`.
The raw score follows UCI root-side perspective; the browser normalizes it to
White's perspective for display. `source` is `search`, `book`, or `tablebase`;
book responses may also include an `opening` name.

## Errors

Invalid input returns HTTP 400. Per-instance request limiting returns HTTP 429.
Engine startup, timeout, a full engine queue, protocol, or process errors
return HTTP 503 with:

```json
{ "error": "Human-readable message" }
```

The service does not accept executable paths, shell commands, or arbitrary UCI
commands through HTTP.
