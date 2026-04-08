import React, { useMemo } from 'react';
import GreenhouseChart from '../GreenhouseTable/GreenhouseChart'; 
import './sidebar.css';
import { Chart as ChartJS, registerables } from 'chart.js';
import 'chartjs-adapter-date-fns';
import zoomPlugin from 'chartjs-plugin-zoom';

ChartJS.register(...registerables, zoomPlugin);

const GreenhouseSidebar = ({ isOpen, records, selectedHours }) => {
  const timeUnit = selectedHours <= 1 ? 'minute' : (selectedHours <= 48 ? 'hour' : 'day');

  const createConfig = (unit) => ({
    responsive: true,
    maintainAspectRatio: false,
    elements: {
      point: {
        radius: 0, 
        hoverRadius: 6, 
        hitRadius: 7, 
      }
    },
    plugins: {
      legend: {
        display: true,
        position: 'top',
        align: 'start',
        labels: { boxWidth: 40, boxHeight: 2, padding: 1, font: { size: 22 }, color: 'lightgrey' }
      },
      zoom: {
        limits: { x: { min: 'original', max: 'original' }, y: { min: 'original', max: 'original' } },
        pan: { enabled: true, mode: 'xy' },
        zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'xy' }
      }
    },
    scales: {
      x: {
        type: 'time',
        time: { unit: unit, displayFormats: { minute: 'h:mm', hour: 'ha', day: 'MMM d:ha' } },
        ticks: { color: 'lightgrey', font: { size: 14 } },
        grid: { color: 'rgba(255,255,255,0.1)' }
      },
      y: {
        ticks: { color: 'lightgrey', font: { size: 14 } },
        grid: { color: 'rgba(255,255,255,0.1)' }
      }
    }
  });

  const optTemp = useMemo(() => createConfig(timeUnit), [timeUnit]);
  const optRSSI = useMemo(() => createConfig(timeUnit), [timeUnit]);

  const labels = records.map(r => new Date(r.datetime));

  return (
    <div className={`sidebar ${isOpen ? 'open' : ''}`}>
      <div className="sidebarContent">
        <div className="chartContainer" id="tempChart">
          <GreenhouseChart
            labels={labels}
            datasets={[
              {
                label: "High Temps °C",
                color: "red",
                data: records.map(r => r.tempHigh),
              },
              {
                label: "Low Temps °C",
                color: "pink",
                data: records.map(r => r.tempLow),
              }
            ]}
            options={optTemp}
          />
        </div>
        <div className="chartContainer">
          <GreenhouseChart
            labels={labels}
            datasets={[
              {
                label: "High RSSI greenhouse",
                color: "blue",
                data: records.map(r => r.rssiHigh)
              },
              {
                label: "Low RSSI greenhouse",
                color: "cyan",
                data: records.map(r => r.rssiLow)
              },
              {
                label: "High RSSI remote",
                color: "green",
                data: records.map(r => r.rssiHighNoDish)
              },
              {
                label: "Low RSSI remote",
                color: "lightgreen",
                data: records.map(r => r.rssiLowNoDish)
              }
            ]}
            options={optRSSI}
          />
        </div>
      </div>
    </div>
  );
};

export default GreenhouseSidebar;