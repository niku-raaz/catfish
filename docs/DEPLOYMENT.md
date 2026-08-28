# Deploying Catfish

## Recommended architecture

Deploy Catfish as one long-running Docker web service:

```text
managed HTTPS / custom domain
             |
  Node + Express on 0.0.0.0:$PORT
       |                 |
  static React UI     /api/search
                         |
             persistent catfish_uci process
                         |
                  catfish_core
```

This preserves same-origin requests and keeps Catfish responsible for every
engine move. No database, object storage, or persistent volume is required for
the default application.

Static-only and request-scoped serverless hosts are not suitable for the
combined deployment because the bridge needs a persistent native child
process. Splitting the frontend onto such a host is possible, but adds CORS,
two deploys, and no product benefit at the current scale.

## Where to deploy

| Platform | Best use | Tradeoff |
| --- | --- | --- |
| Render | Recommended first deployment; simple Git + Docker workflow, managed TLS, health checks, Singapore region | Free resources can be slow for CPU-heavy searches; upgrade for dependable latency |
| Railway | Fast dashboard workflow and automatic Dockerfile detection | Usage-based cost and health checks are configured in the dashboard |
| Fly.io | More control over regions, Machines, and horizontal scaling | More CLI and infrastructure configuration |
| Docker VPS | Sustained CPU or large local Syzygy storage | You maintain OS security, TLS, monitoring, and backups |

Render officially supports root Dockerfile builds, managed TLS, custom
domains, zero-downtime deploys, and Blueprint infrastructure-as-code. Web
services must bind to `0.0.0.0` and the injected `PORT`; Catfish supports both.

## Pre-deployment verification

From the repository root:

```bash
npm ci
npm run check
npm run test:e2e
docker compose up --build
```

In a second terminal:

```bash
curl --fail http://127.0.0.1:8787/api/health
curl --fail http://127.0.0.1:8787/
```

The health response must report:

```json
{"status":"ready","engine":"Catfish"}
```

Stop the local container with `Ctrl+C`.

## Render: step-by-step

### 1. Put the repository on GitHub

Review and commit the changes, then push the `main` branch:

```bash
git status
git add .
git commit -m "Prepare Catfish for production deployment"
git push origin main
```

The included `.github/workflows/ci.yml` verifies C++, TypeScript, unit tests,
Chromium gameplay, production builds, and the Docker image.

### 2. Create the Render service

1. Sign in to the [Render Dashboard](https://dashboard.render.com/).
2. Choose **New → Blueprint**.
3. Connect the GitHub repository containing Catfish.
4. Select the root `render.yaml`.
5. Review the proposed `catfish-chess-engine` web service.
6. Confirm the Singapore region or select another region before creation.
7. Apply the Blueprint.

Render builds the root `Dockerfile`, injects `PORT`, waits for
`/api/health` to return HTTP 200, and then exposes the service at an HTTPS
`onrender.com` address.

The Blueprint starts with the free plan to avoid an unexpected charge. For a
public launch, select a paid instance with meaningful dedicated CPU: chess
search is CPU-bound, and low-CPU/free instances can have inconsistent move
latency.

### 3. Verify the deployed service

In Render:

1. Open **Deploys** and confirm the Docker build completed.
2. Open **Logs** and find `Catfish bridge listening`.
3. Visit `https://<service>.onrender.com/api/health`.
4. Open the service URL and play moves as both White and Black.
5. Confirm the engine source shows `Opening book` or search/hash information.

Command-line check:

```bash
curl --fail https://<service>.onrender.com/api/health
```

### 4. Add a custom domain

1. Open the Render service's **Settings → Custom Domains**.
2. Add the desired domain, such as `chess.example.com`.
3. Add the DNS record Render displays at the domain registrar.
4. Wait for DNS verification and Render's managed TLS certificate.
5. Verify both the homepage and `/api/health` through the custom domain.

### 5. Automatic deployments

`render.yaml` uses `autoDeployTrigger: checksPass`. A push to the linked branch
deploys only after the GitHub **Verify** workflow passes. If a release behaves
badly, use Render's rollback control to restore the previous image.

## Railway alternative

1. Create a Railway project and choose **Deploy from GitHub repo**.
2. Select the Catfish repository; Railway detects the root Dockerfile.
3. Generate a public domain for the service.
4. Set the health-check path to `/api/health`.
5. Leave `PORT` platform-managed.
6. Set `NODE_ENV=production` and
   `CATFISH_ENGINE_PATH=/app/bin/catfish_uci`.
7. Deploy and verify `/api/health`.

Railway injects `PORT` for the application and its deployment health check.

## Fly.io alternative

1. Install `flyctl` and sign in.
2. From the repository root, run:

   ```bash
   fly launch --no-deploy
   ```

3. Choose the closest available region and let Fly detect the Dockerfile.
4. Configure the HTTP service's internal port as `8787`.
5. Configure an HTTP health check for `/api/health`.
6. Set production variables:

   ```bash
   fly secrets set CATFISH_ENGINE_PATH=/app/bin/catfish_uci
   ```

7. Deploy:

   ```bash
   fly deploy
   ```

8. Verify with `fly logs`, `fly status`, and the assigned HTTPS URL.

Fly deploys Dockerfile applications as Machines using the local `fly.toml`
configuration.

## Production configuration

Recommended starting values:

```text
NODE_ENV=production
CATFISH_ENGINE_PATH=/app/bin/catfish_uci
CATFISH_HASH_MB=32
CATFISH_MAX_PENDING_SEARCHES=8
CATFISH_RATE_LIMIT_MAX=60
CATFISH_RATE_LIMIT_WINDOW_MS=60000
```

Increasing `CATFISH_HASH_MB` can improve reuse but consumes that amount per
engine process. Each instance currently owns one engine and serializes its
searches. Scale horizontally for multiple simultaneous searches, or implement
an explicit process pool before placing several engines on one machine.

The in-process rate limiter protects accidental abuse but is per-instance. For
a high-traffic public deployment, add platform/CDN rate limiting and
monitoring.

## Syzygy deployment

The default image does not bundle Fathom or tablebase files. Small local tables
can be added later with a Fathom-enabled image and mounted persistent storage.
Complete seven-piece data is far too large for ordinary app-service disks.
Keep `CATFISH_SYZYGY_PATH` unset until the binary and files are both present.

## Troubleshooting

### Health check returns 503

Read the service logs. The usual cause is an incorrect
`CATFISH_ENGINE_PATH` or a native binary that cannot execute on the target
architecture.

### Service fails port detection

Do not hardcode the public platform port. Leave `PORT` injected by the host and
ensure `NODE_ENV=production`; Catfish then listens on `0.0.0.0:$PORT`.

### Moves time out

Use a Release image, reduce UI depth, or choose an instance with more CPU.
Increasing hash memory does not compensate for severely limited CPU.

### Opening moves search instead of using the book

Check the engine output for `info string book`. External book files must exist
inside the container and be configured through `CATFISH_BOOK_PATH`.
