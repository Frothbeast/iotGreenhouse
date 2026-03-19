import React from 'react';
import './GreenhouseTable.css';

const GreenhouseTable = ({ records = [], columnStats }) => {
    if (!columnStats) return null;

    return (
        <div className="greenhouseTableContainer">
            <table className="greenhouseTable">
                <thead className="greenhouseTableHeader">
                    <tr className="greenhouseTableHeaderRow1">
                        <th className="greenhouseTableHeaderCell1Row1"></th>
                        <th className="greenhouseTableHeaderCell2Row1">TIME</th>
                        <th className="greenhouseTableHeaderCellRow1">TEMP</th>
                        <th className="greenhouseTableHeaderCellRow1">RSSI</th>
                    </tr>
                    <tr className="greenhouseTableHeaderRow2">
                        <th className="greenhouseTableHeaderCell1Row2">MAX</th>
                        <th className="greenhouseTableHeaderCell2Row2">--</th>
                        <th className="greenhouseTableHeaderCellRow2">{columnStats.temp.max}°</th>
                        <th className="greenhouseTableHeaderCellRow2">--</th>
                    </tr>
                    <tr className="greenhouseTableHeaderRow3">
                        <th className="greenhouseTableHeaderCell1Row3">MIN</th>
                        <th className="greenhouseTableHeaderCell2Row3">--</th>
                        <th className="greenhouseTableHeaderCellRow3">{columnStats.temp.min}°</th>
                        <th className="greenhouseTableHeaderCellRow3">--</th>
                    </tr>
                </thead>
                <tbody className="greenhouseTableBody">
                    <tr className="greenhouseTablePlaceholder"></tr>
                    {Array.isArray(records) && records.map((record) => (
                        <tr key={record.id} className="greenhouseTableRow">
                            <td className="greenhouseTableCell1"></td>
                            <td className="greenhouseTableCell2">
                                {record.timestamp ? record.timestamp.split(/[ T]/)[1].substring(0, 5) : "N/a"}
                            </td>
                            <td className="greenhouseTableCell">{record.temp_current ?? "N/a"}°</td>
                            <td className="greenhouseTableCell">{record.rssi_current ?? "N/a"}</td>
                        </tr>
                    ))}
                </tbody>
            </table>
        </div>
    );
};

export default GreenhouseTable;