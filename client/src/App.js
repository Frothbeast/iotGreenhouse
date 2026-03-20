import React, { useState, useMemo, useEffect } from 'react';
import { useGreenhouseData } from './hooks/useGreenhouseData';
import GreenhouseSidebar from './components/sidebar/greenhouseSidebar';
import ControlBar from './components/ControlBar/ControlBar';
import { calculateColumnStats  } from './utils/greenhouseStats';
import './App.css';

function App() {
  const [selectedHours, setSelectedHours] = useState(24);
  const [isSidebarOpen, setSidebarOpen] = useState(false);
  
  // 1. Destructure GreenRecords (matching your hook's return)
  const { GreenRecords, isLoading } = useGreenhouseData(selectedHours);

  // 2. Pass GreenRecords to the stats calculator
  const columnStats = useMemo(() => calculateColumnStats (GreenRecords), [GreenRecords]);

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

  if (isLoading) return <div className="loader">Loading Greenhouse Data...</div>;

  return (
    <div className="App">
      <ControlBar
        selectedHours={selectedHours}
        onHoursChange={setSelectedHours}
        columnStats={columnStats}
        // 3. Pass GreenRecords to ControlBar
        records={GreenRecords} 
        toggleSidebar={() => setSidebarOpen(!isSidebarOpen)}
        isSidebarOpen={isSidebarOpen}
        serverTime={serverTime}
      />
      <main>
        {/* GreenhouseSidebar also needs the correct records variable */}
        <GreenhouseSidebar 
          isOpen={isSidebarOpen} 
          records={GreenRecords} 
          selectedHours={selectedHours} 
        />
      </main>
    </div>
  );
}

export default App;