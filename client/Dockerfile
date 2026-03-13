# ---- Builder ----
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++-13 make \
    && ln -sf /usr/bin/g++-13 /usr/bin/g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

COPY deps/ deps/
COPY common/ common/
COPY client/ client/

WORKDIR /build/client
RUN make

# ---- Runtime ----
FROM ubuntu:24.04

WORKDIR /app
COPY --from=builder /build/client/client_app .

# Interactive CLI — run with: docker compose run --rm client
CMD ["./client_app"]
