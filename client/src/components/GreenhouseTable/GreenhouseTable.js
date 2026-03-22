import React from 'react';
import './GreenhouseTable.css';

const GreenhouseTable = ({ records = [], columnStats }) => {
    if (!columnStats) return null;

    return (
        <div className="greenhouseTableContainer">
            <table className="greenhouseTable">
                <thead className="greenhouseTableHeader">
                    <tr className="greenhouseTableHeaderRow1">
                        <th className="greenhouseTableHeaderCell1Row2"></th>
                        <th className="greenhouseTableHeaderCellRow1 smaller stack-text" id="">tempHigh °C</th>
                        <th className="greenhouseTableHeaderCellRow1 smaller stack-text">tempLow °C</th>
                        <th className="greenhouseTableHeaderCellRow1 smaller stack-text">rssiHigh dB</th>
                        <th className="greenhouseTableHeaderCellRow1 smaller stack-text">rssiLow dB</th>
                        <th className="greenhouseTableHeaderCellRow1 smaller stack-text">rssiHigh NoDish dB</th>
                        <th className="greenhouseTableHeaderCellRow1 smaller stack-text">rssiLow NoDish dB</th>
                        <th className="greenhouseTableHeaderCellRow1 smaller stack-text">Reading Counts</th>
                    </tr>
                    <tr className="greenhouseTableHeaderRow2">
                        <th className="greenhouseTableHeaderCell1Row2 smaller">MAX</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.tempHigh.max}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.tempLow.max}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiHigh.max}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiLow.max}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiHighNoDish.max}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiLowNoDish.max}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.readingCount.max}</th>
                    </tr>
                    <tr className="greenhouseTableHeaderRow3">
                        <th className="greenhouseTableHeaderCell1Row2 smaller">AVG</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.tempHigh.avg}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.tempLow.avg}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiHigh.avg}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiLow.avg}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiHighNoDish.avg}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiLowNoDish.avg}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.readingCount.avg}</th>
                    </tr>
                    <tr className="greenhouseTableHeaderRow3">
                        <th className="greenhouseTableHeaderCell1Row2 smaller">MIN</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.tempHigh.min}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.tempLow.min}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiHigh.min}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiLow.min}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiHighNoDish.min}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiLowNoDish.min}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.readingCount.min}</th>
                    </tr>
                </thead>
                <tbody className="greenhouseTableBody">
                    <tr className="greenhouseTablePlaceholder"></tr>
                    {Array.isArray(records) && records.map((record) => (
                        <tr key={record.id} className="greenhouseTableRow">
                            <td className="greenhouseTableCell2">
                                {record.datetime ? record.datetime.split(/[ T]/)[1].substring(0, 5) : "N/a"}
                            </td>
                            <td className="greenhouseTableCell">{record.tempHigh ?? "N/a"}</td>
                            <td className="greenhouseTableCell">{record.tempLow ?? "N/a"}</td>
                            <td className="greenhouseTableCell">{record.rssiHigh ?? "N/a"}</td>
                            <td className="greenhouseTableCell">{record.rssiLow ?? "N/a"}</td>
                            <td className="greenhouseTableCell">{record.rssiHighNoDish ?? "N/a"}</td>
                            <td className="greenhouseTableCell">{record.rssiLowNoDish ?? "N/a"}</td>
                            <td className="greenhouseTableCell">{record.readingCount ?? "N/a"}</td>
                        </tr>
                    ))}
                </tbody>
            </table>
        </div>
    );
};

export default GreenhouseTable;