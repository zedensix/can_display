# The web page (web/), self-hosted: Bun serves it on PORT (3000), plain
# http, and stores the logs in the /data volume (SQLite). Put it behind
# the https reverse proxy, which also asks who's there: Web Bluetooth
# refuses anything but https, and the server has no login of its own.
FROM oven/bun:1.4.4-alpine

WORKDIR /app
COPY package.json bun.lock bunfig.toml ./
RUN bun install --frozen-lockfile --production
COPY 86.png ./
COPY web ./web

ENV NODE_ENV=production PORT=3000 DATA_DIR=/data
RUN mkdir /data && chown bun:bun /data
VOLUME /data
EXPOSE 3000
USER bun
CMD ["bun", "web/server.ts"]
