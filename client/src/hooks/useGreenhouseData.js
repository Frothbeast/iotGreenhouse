import { useState, useEffect } from 'react';

export function uuseGreenhouseData(hours) {
    const [sumpRecords, setSumpRecords] = useState([]);
    const [isLoading, setIsLoading] = useState(true);

    useEffect(() => {
        let interval;

        const fetchData = () => {
            fetch(`/api/greenhouseData?hours=${hours}`)
                .then(res => res.json())
                .then(data => {
                    if (Array.isArray(data)) {
                        setSumpRecords(data);
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

    return { sumpRecords, isLoading };
}