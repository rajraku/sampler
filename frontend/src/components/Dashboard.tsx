import { useSSE } from '../hooks/useSSE';
import { SensorCard } from './SensorCard';

export function Dashboard() {
  const { data, connected, error } = useSSE();

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
        gridTemplateColumns: 'repeat(auto-fill, minmax(220px, 1fr))',
        gap: '16px'
      }}>
        {Object.values(data).length === 0 ? (
          <div style={{ color: '#6b7280', padding: '20px' }}>
            Waiting for sensor data...
          </div>
        ) : (
          Object.values(data).map(sensor => (
            <SensorCard key={sensor.sensor_id} sensor={sensor} />
          ))
        )}
      </div>
    </div>
  );
}
