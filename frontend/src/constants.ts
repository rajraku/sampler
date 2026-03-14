// Base URL for all server requests.
// Empty string = relative URL (works with the vite dev proxy).
// Override with VITE_SERVER_URL in production (e.g. "http://192.168.1.10:8080").
export const SERVER_URL: string = import.meta.env.VITE_SERVER_URL ?? '';

// API paths — must match server/constants.hh
export const PATH_STREAM = '/stream';

// SSE event names — must match server/constants.hh
export const SSE_EVENT_SENSOR_UPDATE = 'sensor_update';
export const SSE_EVENT_HEARTBEAT     = 'heartbeat';
