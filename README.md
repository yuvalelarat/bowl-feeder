# Food Dispenser GUI

A Node.js + WebSocket + Puppeteer app that visualizes the fill level of a food dispenser bowl in real time.

## Features

- **WebSocket Server:** Streams serial data to the browser.
- **Puppeteer Integration:** Scrapes a web page for serial output.
- **Live Visualization:** `index.html` displays a bowl with animated "food balls" representing the fill percentage.
- **Status Display:** Shows "FULL", "FEEDING", or percentage full.

## How It Works

1. **Backend (`app.js`):**
   - Connects to a Chromium instance via Puppeteer.
   - Loads a page (URL set in `.env` as `URL`).
   - Extracts serial output from `.code_panel__serial__content__text` elements.
   - Broadcasts serial lines to all connected WebSocket clients.

2. **Frontend (`index.html`):**
   - Connects to the backend WebSocket.
   - Updates the bowl visualization and status text based on incoming serial data.

## Setup

1. **Install Dependencies:**
   ```sh
   npm install
   ```

2. **Set Up `.env`:**
   ```
   URL=http://your-serial-output-page
   ```

3. **Start Chromium in Remote Debugging Mode:**
   ```sh
   "C:\Path\To\chrome.exe" --remote-debugging-port=9222 --user-data-dir="C:\Users\{PROFILE_USER}\puppeteer-profile"
   ```

4. **Run the App:**
   ```sh
   node app.js
   ```

5. **Open the Visualization:**
   - Go to [http://localhost:3000](http://localhost:3000) in your browser.

## File Overview

- `app.js` — Node.js server, WebSocket, Puppeteer logic.
- `index.html` — Visualization UI.
- `.env` — Set the `URL` to the page with serial output.

## Requirements

- Node.js
- Chrome/Chromium (for Puppeteer)
- WebSocket-compatible browser

## License

MIT