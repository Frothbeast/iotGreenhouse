import React, { useState, useMemo, useEffect } from 'react';
import { useGreenhouseData } from './hooks/useGreenhouseData';
import GreenhouseSidebar from './components/sidebar/greenhouseSidebar';
import ControlBar from './components/ControlBar/ControlBar';
import { calculateGreenhouseStats } from './utils/greenhouseStats';
import './App.css';

function App() {
  const [selectedHours, setSelectedHours] = useState(24);
  const [isSidebarOpen, setSidebarOpen] = useState(false);
  
  const { records, isLoading } = useGreenhouseData(selectedHours);

  const columnStats = useMemo(() => calculateGreenhouseStats(records), [records]);

  const [serverTime, setServerTime] = useState("00:00 AM");

  const updateTime = () => {
    fetch('/api/time')
      .then(res => res.json())
      .then(data => setServerTime(data.time))
      .catch(err => console.error("Time fetch failed", err));
  };

  useEffect(() => {
    updateTime();
    const interval = setInterval(updateTime, 60000);
    return () => clearInterval(interval);
  }, []);

  if (isLoading) return <div className="loader">Loading...</div>;

  return (
    <div className="App">
      <ControlBar
        selectedHours={selectedHours}
        onHoursChange={setSelectedHours}
        columnStats={columnStats}
        records={records} 
        toggleSidebar={() => setSidebarOpen(!isSidebarOpen)}
        isSidebarOpen={isSidebarOpen}
        serverTime={serverTime}
      />
      <main>
        {/* Greenhouse specific sidebar (mimics Sump) */}
        <GreenhouseSidebar 
          isOpen={isSidebarOpen} 
          records={records} 
          selectedHours={selectedHours} 
        />
        <div className="tableWrapper">
          {/* Greenhouse data table would go here */}
        </div>
      </main>
    </div>
  );
}

export default App;