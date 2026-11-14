# The web page (web/), self-hosted: Bun serves it on PORT (3000), plain
# http. Put it behind the https reverse proxy: Web Bluetooth refuses
# anything else.
FROM oven/bun:1.4.4-alpine

WORKDIR /app
COPY package.json bun.lock bunfig.toml ./
RUN bun install --frozen-lockfile --production
COPY 86.png ./
COPY web ./web

ENV NODE_ENV=production PORT=3000
EXPOSE 3000
USER bun
CMD ["bun", "web/server.ts"]
