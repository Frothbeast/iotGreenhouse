// greenhouseStats.js - FIXED VERSION
export const calculateGreenhouseStats = (records) => {
  if (!records?.length) return null;

  // Use 'temp_current' (mapped from 'temperature' in your hook)
  const temps = records.map(r => parseFloat(r.temp_current)).filter(v => !isNaN(v));
  const rssis = records.map(r => parseInt(r.rssi_current)).filter(v => !isNaN(v));
  
  const lastRecord = records[0];
  const dateObj = new Date(lastRecord.timestamp);

  return {
    temp: {
      avg: (temps.reduce((a, b) => a + b, 0) / temps.length).toFixed(1),
      max: Math.max(...temps).toFixed(1),
      min: Math.min(...temps).toFixed(1)
    },
    rssi: {
      current: lastRecord.rssi_current,
      avg: (rssis.reduce((a, b) => a + b, 0) / rssis.length).toFixed(0)
    },
    lastTime: dateObj.toLocaleTimeString([], { hour: 'numeric', minute: '2-digit' }),
    lastDate: dateObj.toLocaleDateString(),
    count: records.length // Use array length since you don't have a 'reading_count' column
  };
};