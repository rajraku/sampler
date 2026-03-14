import { useState, useEffect } from 'react';
import type { SensorData } from '../types/sensor';
import { SERVER_URL, PATH_STREAM, SSE_EVENT_SENSOR_UPDATE, SSE_EVENT_HEARTBEAT } from '../constants';

export function useSSE(sensors: string[] = []) {
  const [data, setData] = useState<Record<string, SensorData>>({});
  const [connected, setConnected] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    const sensorParam = sensors.length > 0 ? `?sensors=${sensors.join(',')}` : '';
    const url = `${SERVER_URL}${PATH_STREAM}${sensorParam}`;

    console.log('Connecting to SSE:', url);

    const eventSource = new EventSource(url);

    eventSource.onopen = () => {
      console.log('SSE connection opened');
      setConnected(true);
      setError(null);
    };

    eventSource.addEventListener(SSE_EVENT_SENSOR_UPDATE, (e: MessageEvent) => {
      try {
        const sensorData: SensorData = JSON.parse(e.data);
        setData(prev => ({
          ...prev,
          [sensorData.sensor_id]: sensorData
        }));
      } catch (err) {
        console.error('Failed to parse sensor update:', err);
      }
    });

    eventSource.addEventListener(SSE_EVENT_HEARTBEAT, (e: MessageEvent) => {
      console.log('Heartbeat:', e.data);
    });

    eventSource.onerror = (err) => {
      console.error('SSE connection error:', err);
      setConnected(false);
      setError('Connection lost');
    };

    return () => {
      console.log('Closing SSE connection');
      eventSource.close();
    };
  }, [sensors.join(',')]);

  return { data, connected, error };
}
