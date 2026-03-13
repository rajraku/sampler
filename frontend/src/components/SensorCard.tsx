import type { SensorData } from '../types/sensor';

interface DeviceCardProps {
  deviceName: string;
  sensors: SensorData[];
}

const SENSOR_COLOR: Record<string, string> = {
  speed: '#3b82f6',
  temperature: '#ef4444',
  weight: '#10b981',
};

const SENSOR_ICON: Record<string, string> = {
  speed: '⚡',
  temperature: '🌡',
  weight: '⚖',
};

function formatTimestamp(timestamp: string) {
  return new Date(timestamp).toLocaleTimeString();
}

export function SensorCard({ deviceName, sensors }: DeviceCardProps) {
  const latest = sensors.reduce((a, b) =>
    new Date(a.timestamp) > new Date(b.timestamp) ? a : b
  );

  return (
    <div style={{
      border: '1px solid #e5e7eb',
      borderRadius: '10px',
      padding: '20px',
      backgroundColor: '#fff',
      boxShadow: '0 1px 4px rgba(0,0,0,0.06)',
      boxSizing: 'border-box',
      width: '100%',
    }}>
      <div style={{
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        marginBottom: '16px',
      }}>
        <h3 style={{ margin: 0, fontSize: '16px', fontWeight: 600 }}>{deviceName}</h3>
        <span style={{ fontSize: '11px', color: '#9ca3af' }}>
          {formatTimestamp(latest.timestamp)}
        </span>
      </div>

      <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
        {sensors.map(sensor => {
          const color = SENSOR_COLOR[sensor.sensor_type] ?? '#6b7280';
          const icon = SENSOR_ICON[sensor.sensor_type] ?? '●';
          return (
            <div key={sensor.sensor_id} style={{
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'space-between',
              padding: '8px 12px',
              borderRadius: '6px',
              backgroundColor: `${color}12`,
              borderLeft: `3px solid ${color}`,
            }}>
              <span style={{ fontSize: '13px', color: '#4b5563' }}>
                {icon} {sensor.sensor_type}
              </span>
              <span style={{ fontWeight: 700, fontSize: '18px', color }}>
                {sensor.value.toFixed(1)}
                <span style={{ fontWeight: 400, fontSize: '12px', color: '#6b7280', marginLeft: '4px' }}>
                  {sensor.unit}
                </span>
              </span>
            </div>
          );
        })}
      </div>
    </div>
  );
}
