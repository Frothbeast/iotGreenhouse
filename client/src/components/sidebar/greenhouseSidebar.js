import React, { useMemo } from 'react';
import SumpChart from '../sumpTable/sumpChart'; // Reusing your existing chart component
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
        time: { unit: unit, displayFormats: { minute: 'h:mm a', hour: 'MMM d, h a', day: 'MMM d' } },
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

  // [Correction]: Mapping to greenhouseData flat columns
  const labels = records.map(r => new Date(r.timestamp));

  return (
    <div className={`sidebar ${isOpen ? 'open' : ''}`}>
      <div className="sidebarContent">
        <div className="chartContainer">
          <SumpChart
            labels={labels}
            datasets={[
              { label: "Current Temp", color: "#82ca9d", data: records.map(r => r.temp_current) },
              { label: "High", color: "#ff4d4d", data: records.map(r => r.temp_high) },
              { label: "Low", color: "#8884d8", data: records.map(r => r.temp_low) }
            ]}
            options={optTemp}
          />
        </div>
        <div className="chartContainer">
          <SumpChart
            labels={labels}
            datasets={[
              { label: "RSSI", color: "cyan", data: records.map(r => r.rssi_current) }
            ]}
            options={optRSSI}
          />
        </div>
      </div>
    </div>
  );
};

export default GreenhouseSidebar;