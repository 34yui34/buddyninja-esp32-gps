const express = require('express');
const cors = require('cors');
const path = require('path');

const app = express();
const PORT = process.env.PORT || 3000;

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.static('public'));

// In-memory storage (resets when server restarts)
let locationHistory = [];

// API endpoint to receive data from ESP32
app.post('/api/data', (req, res) => {
    console.log('Received data from ESP32:', req.body);
    
    try {
        const { id, payload, date, time } = req.body;
        
        // Decode the hex payload
        const decoded = decodePayload(payload);
        
        // Store the decoded data
        const processedData = {
            id,
            longitude: decoded.longitude,
            latitude: decoded.latitude,
            battery: decoded.battery,
            date,
            time,
            timestamp: new Date().toISOString()
        };
        
        locationHistory.push(processedData);
        
        // Keep only last 100 entries to prevent memory issues
        if (locationHistory.length > 100) {
            locationHistory.shift();
        }
        
        console.log('Decoded data:', processedData);
        
        res.json({ 
            status: 'success', 
            message: 'Data received and decoded',
            decoded: processedData
        });
    } catch (error) {
        console.error('Error processing data:', error);
        res.status(400).json({ 
            status: 'error', 
            message: error.message 
        });
    }
});

// API endpoint to get all location history
app.get('/api/data', (req, res) => {
    res.json({
        status: 'success',
        count: locationHistory.length,
        data: locationHistory
    });
});

// API endpoint to get latest data
app.get('/api/data/latest', (req, res) => {
    if (locationHistory.length === 0) {
        res.json({ 
            status: 'success', 
            message: 'No data yet',
            data: null
        });
    } else {
        res.json({ 
            status: 'success', 
            data: locationHistory[locationHistory.length - 1] 
        });
    }
});

// API endpoint to clear history
app.delete('/api/data', (req, res) => {
    locationHistory = [];
    res.json({ 
        status: 'success', 
        message: 'History cleared' 
    });
});

// Function to decode hex payload
function decodePayload(hexPayload) {
    hexPayload = hexPayload.replace(/\s/g, '').toUpperCase();
    
    if (hexPayload.length !== 10) {
        throw new Error('Invalid payload length. Expected 10 hex characters.');
    }
    
    const lonHex = hexPayload.substring(0, 4);
    const latHex = hexPayload.substring(4, 8);
    const batHex = hexPayload.substring(8, 10);
    
    const longitude = parseInt(lonHex, 16);
    const latitude = parseInt(latHex, 16);
    const battery = parseInt(batHex, 16);
    
    // Convert to realistic GPS coordinates
    const lonDecimal = ((longitude / 65535) * 360 - 180).toFixed(6);
    const latDecimal = ((latitude / 65535) * 180 - 90).toFixed(6);
    
    return {
        longitude: lonDecimal,
        latitude: latDecimal,
        battery: battery
    };
}

// Serve the dashboard at root
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// Health check endpoint
app.get('/health', (req, res) => {
    res.json({ 
        status: 'ok', 
        message: 'Server is running',
        dataCount: locationHistory.length
    });
});

app.listen(PORT, () => {
    console.log(`🚀 Server running on port ${PORT}`);
    console.log(`📡 ESP32 should send data to: http://localhost:${PORT}/api/data`);
    console.log(`🌐 Dashboard available at: http://localhost:${PORT}`);
});