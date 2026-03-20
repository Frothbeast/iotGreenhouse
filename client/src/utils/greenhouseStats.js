const StatsLib = {
  avg: (arr) => arr.length ? (arr.reduce((a, b) => a + b, 0) / arr.length) : 0,
  max: (arr) => arr.length ? Math.max(...arr) : 0,
  min: (arr) => arr.length ? Math.min(...arr) : 0,
};


export const calculateColumnStats  = (greenhouseRecords) => {
  if (!records?.length) return null;

  // Use 'temp_current' (mapped from 'temperature' in your hook)
  const esp_IDs = records.map(r => parseFloat(r.esp_ID)).filter(v => !isNaN(v));
  const tempHighs = records.map(r => parseFloat(r.tempHigh)).filter(v => !isNaN(v));
  const rssiHighs = records.map(r => parseInt(r.rssiHigh)).filter(v => !isNaN(v));
  const tempLows = records.map(r => parseFloat(r.tempLow)).filter(v => !isNaN(v));
  const rssiLows = records.map(r => parseInt(r.rssiLow)).filter(v => !isNaN(v));
  const readingCounts = records.map(r => parseInt(r.readingCount)).filter(v => !isNaN(v));

  const lastRecord = records[0];
  const dateObj = new Date(lastRecord.timestamp);

  return {
    tempHigh: {avg: StatsLib.avg(tempHigh).toFixed(1),max: StatsLib.max(tempHigh).toFixed(1),min: StatsLib.min(tempHigh).toFixed(1)},
    tempLow: {avg: StatsLib.avg(tempLow).toFixed(1),max: StatsLib.max(tempLow).toFixed(1),min: StatsLib.min(tempLow).toFixed(1)},
    rssiHigh: {avg: StatsLib.avg(rssiHigh).toFixed(1),max: StatsLib.max(rssiHigh).toFixed(1),min: StatsLib.min(rssiHigh).toFixed(1)},
    rssiLow: {avg: StatsLib.avg(rssiLow).toFixed(1),max: StatsLib.max(rssiLow).toFixed(1),min: StatsLib.min(rssiLow).toFixed(1)},
    readingCount: {avg: StatsLib.avg(readingCount).toFixed(1),max: StatsLib.max(readingCount).toFixed(1),min: StatsLib.min(readingCount).toFixed(1)},

    lastTime: dateObj.toLocaleTimeString([], { hour: 'numeric', minute: '2-digit' }),
    lastDate: dateObj.toLocaleDateString(),
    lastCount: lastRecord.readingCount
  };
};