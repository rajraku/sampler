import type { SensorData } from '../types/sensor';

interface SensorCardProps {
  sensor: SensorData;
}

export function SensorCard({ sensor }: SensorCardProps) {
  const formatTimestamp = (timestamp: string) => {
    const date = new Date(timestamp);
    return date.toLocaleTimeString();
  };

  const getSensorColor = (type: string) => {
    switch (type) {
      case 'speed': return '#3b82f6';
      case 'temperature': return '#ef4444';
      case 'weight': return '#10b981';
      default: return '#6b7280';
    }
  };

  return (
    <div style={{
      border: `2px solid ${getSensorColor(sensor.sensor_type)}`,
      borderRadius: '8px',
      padding: '16px',
      backgroundColor: '#fff',
      minWidth: '200px'
    }}>
      <h3 style={{ margin: '0 0 8px 0', fontSize: '14px', color: '#6b7280' }}>
        {sensor.device_name} - {sensor.sensor_type}
      </h3>
      <div style={{ fontSize: '32px', fontWeight: 'bold', margin: '8px 0' }}>
        {sensor.value.toFixed(1)} <span style={{ fontSize: '16px', color: '#6b7280' }}>{sensor.unit}</span>
      </div>
      <div style={{ fontSize: '12px', color: '#9ca3af' }}>
        Updated: {formatTimestamp(sensor.timestamp)}
      </div>
    </div>
  );
}
