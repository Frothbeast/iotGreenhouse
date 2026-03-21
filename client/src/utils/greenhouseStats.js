const StatsLib = {
  avg: (arr) => arr.length ? (arr.reduce((a, b) => a + b, 0) / arr.length) : 0,
  max: (arr) => arr.length ? Math.max(...arr) : 0,
  min: (arr) => arr.length ? Math.min(...arr) : 0,
};


export const calculateColumnStats  = (greenRecords) => {
  if (!greenRecords?.length) return null;

  // Use 'temp_current' (mapped from 'temperature' in your hook)
  const esp_IDs = greenRecords.map(r => parseFloat(r.esp_ID)).filter(v => !isNaN(v));
  const tempHighs = greenRecords.map(r => parseFloat(r.tempHigh)).filter(v => !isNaN(v));
  const rssiHighs = greenRecords.map(r => parseInt(r.rssiHigh)).filter(v => !isNaN(v));
  const tempLows = greenRecords.map(r => parseFloat(r.tempLow)).filter(v => !isNaN(v));
  const rssiLows = greenRecords.map(r => parseInt(r.rssiLow)).filter(v => !isNaN(v));
  const readingCounts = greenRecords.map(r => parseInt(r.readingCount)).filter(v => !isNaN(v));

  const lastRecord = greenRecords[0];
  const dateObj = new Date(lastRecord.timestamp);

  return {
    tempHigh: {avg: StatsLib.avg(tempHighs).toFixed(1),max: StatsLib.max(tempHighs).toFixed(1),min: StatsLib.min(tempHighs).toFixed(1)},
    tempLow: {avg: StatsLib.avg(tempLows).toFixed(1),max: StatsLib.max(tempLows).toFixed(1),min: StatsLib.min(tempLows).toFixed(1)},
    rssiHigh: {avg: StatsLib.avg(rssiHighs).toFixed(1),max: StatsLib.max(rssiHighs).toFixed(1),min: StatsLib.min(rssiHighs).toFixed(1)},
    rssiLow: {avg: StatsLib.avg(rssiLows).toFixed(1),max: StatsLib.max(rssiLows).toFixed(1),min: StatsLib.min(rssiLows).toFixed(1)},
    readingCount: {avg: StatsLib.avg(readingCounts).toFixed(1),max: StatsLib.max(readingCounts).toFixed(1),min: StatsLib.min(readingCounts).toFixed(1)},

    lastTime: dateObj.toLocaleTimeString([], { hour: 'numeric', minute: '2-digit' }),
    lastDate: dateObj.toLocaleDateString(),
    lastCount: lastRecord.readingCount
  };
};