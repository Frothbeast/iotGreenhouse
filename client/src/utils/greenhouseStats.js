// src/utils/greenhouseStats.js

const StatsLib = {
  avg: (arr) => arr.length ? (arr.reduce((a, b) => a + b, 0) / arr.length).toFixed(1) : 0,
  max: (arr) => arr.length ? Math.max(...arr).toFixed(1) : 0,
  min: (arr) => arr.length ? Math.min(...arr).toFixed(1) : 0,
};

export const calculateGreenhouseStats = (records) => {
  if (!records?.length) return null;

  // [Correction]: Mapping directly to the flat SQL columns in greenhouseData table
  const currentTemps = records.map(r => parseFloat(r.temp_current)).filter(v => !isNaN(v));
  const highTemps = records.map(r => parseFloat(r.temp_high)).filter(v => !isNaN(v));
  const lowTemps = records.map(r => parseFloat(r.temp_low)).filter(v => !isNaN(v));
  
  const lastRecord = records[0];
  
  // Format the SQL timestamp for display
  const dateObj = new Date(lastRecord.timestamp);
  const lastDate = dateObj.toLocaleDateString();
  const lastTime = dateObj.toLocaleTimeString([], { hour: 'numeric', minute: '2-digit', hour12: true });

  return {
    temp: { 
      avg: StatsLib.avg(currentTemps), 
      max: StatsLib.max(highTemps), 
      min: StatsLib.min(lowTemps) 
    },
    rssi: {
      current: lastRecord.rssi_current,
      high: lastRecord.rssi_high,
      low: lastRecord.rssi_low
    },
    lastTime,
    lastDate,
    count: lastRecord.reading_count
  };
};