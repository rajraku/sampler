# IoT Dashboard Frontend

React + TypeScript dashboard for real-time IoT sensor monitoring.

## Setup

```bash
npm install
```

## Development

```bash
npm run dev
```

Open [http://localhost:5173](http://localhost:5173) in your browser.

## Build

```bash
npm run build
```

## Features

- Real-time sensor data via Server-Sent Events (SSE)
- Automatic reconnection on connection loss
- Responsive grid layout
- Color-coded sensor types
- Live timestamps

## Architecture

- **SSE Hook** ([src/hooks/useSSE.ts](src/hooks/useSSE.ts)) - Manages SSE connection and sensor data state
- **Dashboard** ([src/components/Dashboard.tsx](src/components/Dashboard.tsx)) - Main view with connection status
- **SensorCard** ([src/components/SensorCard.tsx](src/components/SensorCard.tsx)) - Individual sensor display component
