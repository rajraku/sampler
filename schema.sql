-- IoT Sensor Data Schema
-- This schema is automatically loaded when PostgreSQL container starts

BEGIN;

-- Append-Only Event Log
-- Stores all sensor readings with complete history
CREATE TABLE IF NOT EXISTS sensor_events (
    id BIGSERIAL PRIMARY KEY,
    sensor_id VARCHAR(100) NOT NULL,
    sensor_type VARCHAR(50) NOT NULL,
    device_name VARCHAR(100) NOT NULL,
    value DOUBLE PRECISION NOT NULL,
    unit VARCHAR(20) NOT NULL,
    timestamp TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Indexes for efficient queries
CREATE INDEX IF NOT EXISTS idx_sensor_events_sensor_id_timestamp
    ON sensor_events(sensor_id, timestamp DESC);

CREATE INDEX IF NOT EXISTS idx_sensor_events_device_timestamp
    ON sensor_events(device_name, timestamp DESC);

CREATE INDEX IF NOT EXISTS idx_sensor_events_type_timestamp
    ON sensor_events(sensor_type, timestamp DESC);

-- Current State Table
-- Stores the latest reading for each sensor (upsert pattern)
CREATE TABLE IF NOT EXISTS sensor_latest (
    sensor_id VARCHAR(100) PRIMARY KEY,
    sensor_type VARCHAR(50) NOT NULL,
    device_name VARCHAR(100) NOT NULL,
    value DOUBLE PRECISION NOT NULL,
    unit VARCHAR(20) NOT NULL,
    timestamp TIMESTAMPTZ NOT NULL,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Indexes for sensor_latest
CREATE INDEX IF NOT EXISTS idx_sensor_latest_device
    ON sensor_latest(device_name);

CREATE INDEX IF NOT EXISTS idx_sensor_latest_type
    ON sensor_latest(sensor_type);

COMMIT;

-- Display created tables
\dt
