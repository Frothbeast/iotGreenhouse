import SumpChart from '../sumpTable/sumpChart'; // Reusing your existing chart component
import './ControlBar.css';

const ControlBar = ({ cl1pClick, selectedHours, onHoursChange, columnStats, records, toggleSidebar, isSidebarOpen, serverTime }) => {
  const getOptions = (min, max) => ({
    responsive: true,
    maintainAspectRatio: false,
    plugins: { legend: { display: false } },
    scales: {
      x: { display: false, reverse: true },
      y: { display: false, min, max }
    },
    elements: { point: { radius: 0 } }
  });

  return (
    <header className="controlBar">
      <div className="brandSection">
        {/* [Correction]: Labeled Greenhouse but keeps Sump styling */}
        <div className="brand">Greenhouse</div>
        <div className="serverTime">
          <span className="stLabel">Server Time:</span>
          <span>{serverTime ?? "00:00:00"}</span>
        </div>
      </div>
      
      <div className="centerSection">
        <div className="lastRun">
          <span className="label">Last Reading</span>
          <span className="value">{columnStats?.lastTime ?? "N/a"}</span>
          <span className="unit">{columnStats?.lastDate ?? "N/a"}</span>
        </div>
        <div className="statsRow">
          <div className="statItem">
            <span className="label">Current Temp</span>
            <span className="value">{columnStats?.temp.avg ?? 0}°</span>
          </div>
          <div className="statItem">
            <span className="label">Count</span>
            <span className="value">{columnStats?.count ?? 0}</span>
          </div>
        </div>
      </div>

      <div className="chartSection">
        <div className="chartContainer">
          <div className="chartWatermark">TEMP</div>
          <SumpChart
            labels={records.map((_, i) => i)}
            datasets={[
              {
                label: "Current Temp",
                color: "#82ca9d",
                data: records.map(r => r.temp_current),
              }
            ]}
            options={getOptions(0, 120)}
          />
        </div>
        <div className="chartContainer">
          <div className="chartWatermark">RSSI</div>
          <SumpChart
            labels={records.map((_, i) => i)}
            datasets={[
              {
                label: "RSSI",
                color: "cyan",
                data: records.map(r => r.rssi_current)
              }
            ]}
            options={getOptions(-100, 0)}
          />
        </div>
      </div>
    </header>
  );
};

export default ControlBar;