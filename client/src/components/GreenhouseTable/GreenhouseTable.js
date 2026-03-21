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
                        <th className="greenhouseTableHeaderCellRow1">tempHigh</th>
                        <th className="greenhouseTableHeaderCellRow1">tempLow</th>
                        <th className="greenhouseTableHeaderCellRow1">rssiHigh</th>
                        <th className="greenhouseTableHeaderCellRow1">rssiLow</th>
                        <th className="greenhouseTableHeaderCellRow1">Hadc</th>
                    </tr>
                    <tr className="greenhouseTableHeaderRow2">
                        <th className="greenhouseTableHeaderCell1Row2">MAX</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.tempHigh.max}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.tempLow.max}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiHigh.max}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiLow.max}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.readingCount.max}</th>
                    </tr>
                    <tr className="greenhouseTableHeaderRow3">
                        <th className="greenhouseTableHeaderCell1Row2">AVG</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.tempHigh.avg}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.tempLow.avg}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiHigh.avg}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiLow.avg}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.readingCount.avg}</th>
                    </tr>
                    <tr className="greenhouseTableHeaderRow3">
                        <th className="greenhouseTableHeaderCell1Row2">MIN</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.tempHigh.min}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.tempLow.min}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiHigh.min}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.rssiLow.min}</th>
                        <th className="greenhouseTableHeaderCellRow1">{columnStats.readingCount.min}</th>
                    </tr>
                </thead>
                <tbody className="greenhouseTableBody">
                    <tr className="greenhouseTablePlaceholder"></tr>
                    {Array.isArray(records) && records.map((record) => (
                        <tr key={record.id} className="greenhouseTableRow">
                            <td className="greenhouseTableCell1"></td>
                            <td className="greenhouseTableCell2">
                                {record.datetime ? record.datetime.split(/[ T]/)[1].substring(0, 5) : "N/a"}
                            </td>
                            <td className="greenhouseTableCell">{record.tempHigh ?? "N/a"}°C</td>
                            <td className="greenhouseTableCell">{record.tempLow ?? "N/a"}°C</td>
                            <td className="greenhouseTableCell">{record.rssiHigh ?? "N/a"}dB</td>
                            <td className="greenhouseTableCell">{record.rssiLow ?? "N/a"}dB</td>
                            <td className="greenhouseTableCell">{record.readingCount ?? "N/a"}</td>
                        </tr>
                    ))}
                </tbody>
            </table>
        </div>
    );
};

export default GreenhouseTable;