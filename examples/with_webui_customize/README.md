# WiFi Manager - Customizable Web UI Example

Example demonstrating WiFi Manager with customizable frontend from SPIFFS.

## Features

- Custom frontend files served from SPIFFS partition
- Replace UI without recompiling firmware
- Fallback to embedded Web UI if files not found
- 512KB SPIFFS partition for frontend files

## How It Works

1. WiFi Manager first checks `/spiffs/` for frontend files
2. If found, serves custom files from SPIFFS
3. If not found, falls back to embedded Web UI

## File Structure

```
/spiffs/
├── index.html           # Main HTML file
└── assets/
    ├── app.js           # JavaScript (or app.js.gz)
    └── index.css        # Styles (or index.css.gz)
```

## Build & Flash

```bash
cd examples/with_webui_customize
idf.py build flash monitor
```

This will:
1. Build the firmware
2. Create SPIFFS image from `www/` directory
3. Flash everything including custom frontend

## Customizing the Frontend

### Option 1: Modify Before Build

Edit files in `www/` directory, then rebuild:

```bash
# Edit www/index.html
idf.py build flash
```

### Option 2: Use Default Frontend as Base

Copy the built frontend from `frontend/dist/`:

```bash
# Build frontend first
cd frontend
npm install
npm run build

# Copy to www folder
cp dist/index.html ../examples/with_webui_customize/www/
cp -r dist/assets ../examples/with_webui_customize/www/

# Rebuild and flash
cd ../examples/with_webui_customize
idf.py build flash
```

### Option 3: Upload via Serial (Advanced)

After initial flash, update only the SPIFFS partition:

```bash
# Create new SPIFFS image
$IDF_PATH/components/spiffs/spiffsgen.py 512000 www storage.bin

# Flash only SPIFFS partition
esptool.py --port /dev/ttyUSB0 write_flash 0x110000 storage.bin
```

## Creating Your Own Frontend

Keep these in mind:

1. **API Base Path**: `/api/wifi`
2. **Endpoints**:
   - `GET /api/wifi/status` - WiFi status
   - `GET /api/wifi/scan` - Scan networks
   - `GET /api/wifi/networks` - Saved networks
   - `POST /api/wifi/networks` - Add network
   - `DELETE /api/wifi/networks/:ssid` - Delete network
   - `POST /api/wifi/connect` - Connect
   - `POST /api/wifi/disconnect` - Disconnect
   - `POST /api/wifi/factory_reset` - Factory reset

3. **Response Format**: JSON
4. **Authentication**: Optional (disabled by default)

### Example API Usage

```javascript
// Get status
const status = await fetch('/api/wifi/status').then(r => r.json());

// Scan networks
const scan = await fetch('/api/wifi/scan').then(r => r.json());

// Add and connect to network
await fetch('/api/wifi/networks', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({
        ssid: 'MyWiFi',
        password: 'secret123',
        priority: 10
    })
});

await fetch('/api/wifi/connect', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({ssid: 'MyWiFi'})
});
```

## Tips for Custom Frontend

1. **Keep it Small**: SPIFFS has 512KB, frontend should be <100KB
2. **Use Gzip**: Name files with `.gz` extension for automatic gzip serving
3. **Cache Headers**: Browser caches assets, use versioned filenames
4. **Mobile First**: Test on mobile devices for captive portal UX
5. **Offline First**: Handle connection drops gracefully

## Partition Table

| Name | Type | Size |
|------|------|------|
| nvs | data | 24KB |
| phy_init | data | 4KB |
| factory | app | 1MB |
| storage | spiffs | 512KB |
