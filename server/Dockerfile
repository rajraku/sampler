# ---- Builder ----
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++-13 make cmake git ca-certificates \
    libboost1.83-dev libpq-dev libhiredis-dev \
    && ln -sf /usr/bin/g++-13 /usr/bin/g++ \
    && rm -rf /var/lib/apt/lists/*

# Build libpqxx from source (static) — Ubuntu 24.04 package has a symbol mismatch
RUN git clone --depth=1 \
        https://github.com/jtv/libpqxx.git /tmp/pqxx \
    && cmake -S /tmp/pqxx -B /tmp/pqxx/build \
        -DBUILD_SHARED_LIBS=OFF \
        -DSKIP_BUILD_TEST=ON \
    && cmake --build /tmp/pqxx/build \
    && cmake --install /tmp/pqxx/build \
    && rm -rf /tmp/pqxx

# Build redis-plus-plus from source (static) — not packaged in Ubuntu 24.04
RUN git clone --depth=1 \
        https://github.com/sewenew/redis-plus-plus.git /tmp/redispp \
    && cmake -S /tmp/redispp -B /tmp/redispp/build \
        -DREDIS_PLUS_PLUS_CXX_STANDARD=20 \
        -DREDIS_PLUS_PLUS_BUILD_SHARED=OFF \
        -DREDIS_PLUS_PLUS_BUILD_STATIC=ON \
        -DREDIS_PLUS_PLUS_BUILD_TEST=OFF \
    && cmake --build /tmp/redispp/build \
    && cmake --install /tmp/redispp/build \
    && rm -rf /tmp/redispp

WORKDIR /build

COPY deps/ deps/
COPY common/ common/
COPY server/ server/

WORKDIR /build/server
RUN make

# ---- Runtime ----
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 libhiredis1.1.0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /build/server/server_app .

EXPOSE 8080
CMD ["./server_app"]
