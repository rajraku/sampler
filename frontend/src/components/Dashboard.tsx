import { useSSE } from '../hooks/useSSE';
import { SensorCard } from './SensorCard';
import type { SensorData } from '../types/sensor';

export function Dashboard() {
  const { data, connected, error } = useSSE();

  // Group sensors by device_name
  const byDevice: Record<string, SensorData[]> = {};
  for (const sensor of Object.values(data)) {
    if (!byDevice[sensor.device_name]) byDevice[sensor.device_name] = [];
    byDevice[sensor.device_name].push(sensor);
  }

  return (
    <div style={{ padding: '20px', fontFamily: 'system-ui, sans-serif' }}>
      <div style={{
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        marginBottom: '24px'
      }}>
        <h1 style={{ margin: 0 }}>IoT Sensor Dashboard</h1>
        <div style={{
          padding: '8px 16px',
          borderRadius: '4px',
          backgroundColor: connected ? '#10b981' : '#ef4444',
          color: 'white',
          fontSize: '14px'
        }}>
          {connected ? '● Connected' : '○ Disconnected'}
        </div>
      </div>

      {error && (
        <div style={{
          padding: '12px',
          backgroundColor: '#fee2e2',
          color: '#991b1b',
          borderRadius: '4px',
          marginBottom: '16px'
        }}>
          {error}
        </div>
      )}

      <div style={{
        display: 'grid',
        gridTemplateColumns: 'repeat(auto-fill, minmax(min(220px, 100%), 1fr))',
        gap: '32px'
      }}>
        {Object.keys(byDevice).length === 0 ? (
          <div style={{ color: '#6b7280', padding: '20px' }}>
            Waiting for sensor data...
          </div>
        ) : (
          Object.entries(byDevice).map(([deviceName, sensors]) => (
            <SensorCard key={deviceName} deviceName={deviceName} sensors={sensors} />
          ))
        )}
      </div>
    </div>
  );
}
