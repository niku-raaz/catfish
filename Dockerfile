# syntax=docker/dockerfile:1

FROM node:22-bookworm-slim AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential cmake ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY package.json package-lock.json ./
RUN npm ci

COPY CMakeLists.txt ./
COPY include ./include
COPY src ./src
COPY tools ./tools

RUN cmake -S . -B build/container \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=OFF \
    && cmake --build build/container --target catfish_uci --parallel

COPY index.html tsconfig.json tsconfig.server.json vite.config.ts ./
COPY public ./public
COPY server ./server
COPY web ./web

RUN npm run typecheck \
    && npm run build:web \
    && npm run build:server

FROM node:22-bookworm-slim AS runtime

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates tini \
    && rm -rf /var/lib/apt/lists/*

ENV NODE_ENV=production \
    PORT=8787 \
    CATFISH_ENGINE_PATH=/app/bin/catfish_uci \
    CATFISH_HASH_MB=32 \
    CATFISH_MAX_PENDING_SEARCHES=8

WORKDIR /app

COPY package.json package-lock.json ./
RUN npm ci --omit=dev \
    && npm cache clean --force

COPY --from=build --chown=node:node /app/build/container/catfish_uci ./bin/catfish_uci
COPY --from=build --chown=node:node /app/dist-server ./dist-server
COPY --from=build --chown=node:node /app/dist-web ./dist-web

USER node
EXPOSE 8787

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
  CMD ["node", "-e", "fetch(`http://127.0.0.1:${process.env.PORT ?? 8787}/api/health`).then(r=>{if(!r.ok)process.exit(1)}).catch(()=>process.exit(1))"]

ENTRYPOINT ["tini", "--"]
CMD ["node", "dist-server/server/index.js"]
