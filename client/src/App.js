import React, { useState, useMemo, useEffect } from 'react';
import { useGreenhouseData } from './hooks/useGreenhouseData';
import GreenhouseSidebar from './components/sidebar/greenhouseSidebar';
import GreenhouseTable from './components/GreenhouseTable/GreenhouseTable';
import ControlBar from './components/ControlBar/ControlBar';
import { calculateColumnStats  } from './utils/greenhouseStats';
import './App.css';

function App() {
  const [selectedHours, setSelectedHours] = useState(24);
  const [isSidebarOpen, setSidebarOpen] = useState(false);
  
  // 1. Destructure GreenRecords (matching your hook's return)
  const { greenRecords, isLoading } = useGreenhouseData(selectedHours);

  // 2. Pass GreenRecords to the stats calculator
  const columnStats = useMemo(() => calculateColumnStats (greenRecords), [greenRecords]);

  const [serverTime, setServerTime] = useState("00:00 AM");

  const cl1pClick = async () => {
    try{
      const response = await fetch(`${process.env.REACT_APP_GREEN_API_URL}/api/cl1p`, {method: 'POST', });
      if (response.status === 204) {
          console.log("Action acknowledged by server.");
      }
      else {
        console.error("Server returned an error");
      }
    }
    catch (error) {
      console.error("Fetch error:", error);
    }
  };

  const updateTime = () => {
    fetch(`${process.env.REACT_APP_GREEN_API_URL}/api/time`)
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
        cl1pClick={cl1pClick}
        records={greenRecords}
        toggleSidebar={() => setSidebarOpen(!isSidebarOpen)}
        isSidebarOpen={isSidebarOpen}
        serverTime={serverTime}
      />
      <main>
        {/* GreenhouseSidebar also needs the correct records variable */}
        <GreenhouseSidebar 
          isOpen={isSidebarOpen} 
          records={greenRecords}
          selectedHours={selectedHours} 
        />
        <GreenhouseTable 
          records={greenRecords} 
          columnStats={columnStats} 
        />
      </main>
    </div>
  );
}

export default App;