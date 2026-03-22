import { useState, useEffect } from 'react';

export function useGreenhouseData(hours) {
    const [greenRecords, setGreenRecords] = useState([]);
    const [isLoading, setIsLoading] = useState(true);
    const API_BASE_URL = process.env.REACT_APP_API_URL || '';
    useEffect(() => {
        let interval;

        const fetchData = () => {
            fetch(`${API_BASE_URL}/api/greenhouseData?hours=${hours}`)
                .then(res => res.json())
                .then(data => {
                    if (Array.isArray(data)) {
                        const mappedData = data.map(r => ({
                            ...r,
                            datetime: r.datetime,
                            tempHigh: r.tempHigh,
                            tempLow: r.tempLow,
                            rssiHigh: r.rssiHigh,
                            rssiLow: r.rssiLowNoDish,
                            rssiHighNoDish: r.rssiHighNoDish,
                            rssiLowNoDIsh: r.rssiLowNoDish,
                            readingCount: r.readingCount
                    }));
                        setGreenRecords(mappedData);
                    }
                    setIsLoading(false);
                })
                .catch(err => {
                    console.error("Fetch error:", err);
                    setIsLoading(false);
                });
        };

        const setupInterval = () => {
            if (interval) clearInterval(interval);
            
            // Poll at 1s if visible, 60s if hidden to save MSI server resources
            const pollRate = document.visibilityState === 'visible' ? 1000 : 60000;
            interval = setInterval(fetchData, pollRate);
        };

        // Initial fetch and start polling
        fetchData();
        setupInterval();

        const handleVisibilityChange = () => {
            setupInterval();
        };

        document.addEventListener('visibilitychange', handleVisibilityChange);

        return () => {
            clearInterval(interval);
            document.removeEventListener('visibilitychange', handleVisibilityChange);
        };
    }, [hours]);

    return { greenRecords, isLoading };
}