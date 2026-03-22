import GreenhouseChart from '../GreenhouseTable/GreenhouseChart';
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
    elements: { 
      point: { 
        radius: 0, 
        hitRadius: 0, 
        hoverRadius: 0 
      } 
    }
  });

  return (
    <header className="controlBar">
      <div className="brandSection">
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
        <div className="buttonRow">
          <button className="sidebarButton" onClick={toggleSidebar}>
            {isSidebarOpen ? "Close Chart" : "View Graph"}
          </button>
          <button onClick={cl1pClick} className="cl1pButton">CL1P</button>
          <select className="selectedHours" value={selectedHours} onChange={(e) => onHoursChange(Number(e.target.value))}>
            <option value={1}>1 Hour</option>
            <option value={8}>8 Hour</option>
            <option value={24}>24 Hour</option>
            <option value={168}>168 Hour</option>
          </select>
        </div>
      </div>

      <div className="chartSection">
        <div className="chartContainer" id="tempChart">
          <div className="chartWatermark">TEMP</div>
          <GreenhouseChart
            labels={records.map((_, i) => i)}
            datasets={[
              {
                label: "High Temps",
                color: "red",
                data: records.map(r => r.tempHigh),
              },
              {
                label: "Low Temps",
                color: "pink",
                data: records.map(r => r.tempLow),
              }
            ]}
            options={getOptions(0, 120)}
          />
        </div>
        <div className="chartContainer">
          <div className="chartWatermark">RSSI</div>
          <GreenhouseChart
            labels={records.map((_, i) => i)}
            datasets={[
              {
                label: "High RSSI",
                color: "cyan",
                data: records.map(r => r.rssiHigh)
              },
              {
                label: "Low RSSI",
                color: "cyan",
                data: records.map(r => r.rssiLow)
              }
            ]}
            options={getOptions(-100, 0)}
          />
        </div>
                <div className="chartContainer">
          <div className="chartWatermark">RSSI NO DISH</div>
          <GreenhouseChart
            labels={records.map((_, i) => i)}
            datasets={[
              {
                label: "High RSSI No Dish",
                color: "cyan",
                data: records.map(r => r.rssiHighNoDish)
              },
              {
                label: "Low RSSI No Dish",
                color: "cyan",
                data: records.map(r => r.rssiLowNoDish)
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