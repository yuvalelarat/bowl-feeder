require('dotenv').config();
const express = require('express');
const path = require('path');
const WebSocket = require('ws');
const puppeteer = require('puppeteer-core');

const app = express();
const PORT = 3000;

app.use(express.static(path.join(__dirname)));

const server = app.listen(PORT, () => {
  console.log(`Server running at http://localhost:${PORT}`);
});

const wss = new WebSocket.Server({ server });

let sockets = [];
wss.on('connection', (ws) => {
  console.log('WebSocket client connected');
  sockets.push(ws);
});

function broadcast(data) {
  sockets.forEach(ws => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify(data));
    }
  });
}

const url = process.env.URL; //from proccess.env.URL';

(async () => {
    const browserURL = 'http://localhost:9222';
    const browser = await puppeteer.connect({ browserURL });
    const pages = await browser.pages();
    const page = pages[0];

    await page.setViewport({ width: 1920, height: 900 });

    await page.goto(url, { waitUntil: 'domcontentloaded' });
    await page.waitForSelector('.code_panel__serial__content', { timeout: 0 });

    await page.exposeFunction('onSerialLine', line => {
        console.log(line);
        broadcast({ type: 'serial', data: line });
    });

    await page.evaluate(() => {
    let lastText = '';

        setInterval(() => {
            const allText = Array.from(document.querySelectorAll('.code_panel__serial__content__text'))
            .map(el => el.innerText)
            .join('\n');

            if (allText !== lastText) {
            const newPart = allText.slice(lastText.length).trim();
            if (newPart) {
                newPart.split('\n').forEach(line => window.onSerialLine(line.trim()));
            }
            lastText = allText;
            }
        }, 300);
    });
})();
