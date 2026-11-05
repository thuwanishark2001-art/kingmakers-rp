/**
 * =====================================================================
 *  KINGMAKERS RP - Backend API & Live Web Portal
 *  Serves client launcher updater, live server status, web dashboard,
 *  leaderboards, economy stats, and admin REST endpoints.
 * =====================================================================
 */

const express = require('express');
const cors = require('cors');
const path = require('path');
const fs = require('fs');
const dgram = require('dgram');
let Database;
try { Database = require('better-sqlite3'); } catch (e) { Database = null; }

const app = express();
const PORT = process.env.PORT || 8080;
const DB_PATH = process.env.DB_PATH || path.join(__dirname, '../server/scriptfiles/kingmakers.db');
const SAMP_HOST = process.env.SAMP_HOST || '127.0.0.1';
const SAMP_PORT = parseInt(process.env.SAMP_PORT || '7777', 10);

app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));
app.use('/assets', express.static(path.join(__dirname, '../assets/production')));

function getDB() {
    if (!Database || !fs.existsSync(DB_PATH)) return null;
    try {
        return new Database(DB_PATH, { readonly: true, fileMustExist: true });
    } catch (e) {
        return null;
    }
}

// ---------------------------------------------------------------------
//  SA-MP / open.mp native UDP query protocol (OpCode 'i' for info)
// ---------------------------------------------------------------------
function querySampServer(host, port, timeout = 1500) {
    return new Promise((resolve) => {
        const socket = dgram.createSocket('udp4');
        const parts = host.split('.').map(x => parseInt(x, 10));
        if (parts.length !== 4) return resolve({ online: false });

        // Packet header: "SAMP" + IP (4 bytes) + Port (2 bytes LE) + Opcode ('i')
        const packet = Buffer.alloc(11);
        packet.write('SAMP', 0);
        packet[4] = parts[0];
        packet[5] = parts[1];
        packet[6] = parts[2];
        packet[7] = parts[3];
        packet.writeUInt16LE(port, 8);
        packet.write('i', 10);

        let timer = setTimeout(() => {
            try { socket.close(); } catch (e) {}
            resolve({ online: false });
        }, timeout);

        socket.on('message', (msg) => {
            clearTimeout(timer);
            try {
                // Header (11) + password (1) + players (2) + max (2) + hostnamelen (4) + hostname...
                if (msg.length >= 15) {
                    const password = msg[11];
                    const players = msg.readUInt16LE(12);
                    const maxPlayers = msg.readUInt16LE(14);
                    const nameLen = msg.readUInt32LE(16);
                    const hostname = msg.toString('utf8', 20, 20 + nameLen);
                    socket.close();
                    return resolve({
                        online: true,
                        password: password === 1,
                        players,
                        maxPlayers,
                        hostname,
                        host,
                        port
                    });
                }
            } catch (e) {}
            try { socket.close(); } catch (e) {}
            resolve({ online: false });
        });

        socket.on('error', () => {
            clearTimeout(timer);
            try { socket.close(); } catch (e) {}
            resolve({ online: false });
        });

        socket.send(packet, 0, packet.length, port, host);
    });
}

// ---------------------------------------------------------------------
//  REST API Endpoints
// ---------------------------------------------------------------------
app.get('/api/status', async (req, res) => {
    const live = await querySampServer(SAMP_HOST, SAMP_PORT);
    const db = getDB();
    let stats = { accounts: 0, vehicles: 0, houses: 0, businesses: 0 };
    if (db) {
        try {
            stats.accounts = db.prepare('SELECT COUNT(*) AS c FROM accounts').get()?.c || 0;
            stats.vehicles = db.prepare('SELECT COUNT(*) AS c FROM vehicles').get()?.c || 0;
            stats.houses = db.prepare('SELECT COUNT(*) AS c FROM houses').get()?.c || 0;
            stats.businesses = db.prepare('SELECT COUNT(*) AS c FROM businesses').get()?.c || 0;
            db.close();
        } catch (e) {}
    }
    res.json({
        serverName: 'Kingmakers RP',
        version: '1.0.0',
        online: live.online,
        players: live.players || 0,
        maxPlayers: live.maxPlayers || 100,
        hostname: live.hostname || 'Kingmakers RP | Mobile Roleplay | v1.0.0',
        host: SAMP_HOST,
        port: SAMP_PORT,
        stats
    });
});

app.get('/api/launcher/config', (req, res) => {
    res.json({
        latestVersion: '1.0.0',
        mandatoryUpdate: false,
        clientApkUrl: 'https://cdn.kingmakers-rp.com/apk/kingmakers_rp_v1.0.apk',
        cacheZipUrl: 'https://cdn.kingmakers-rp.com/cache/km_cache_v1.zip',
        cacheSizeMb: 850,
        cacheSha256: 'a872f52b24b015eac202d7e0453e8a44',
        defaultServer: {
            name: 'Kingmakers RP #1 [Los Santos]',
            ip: SAMP_HOST,
            port: SAMP_PORT
        },
        news: [
            { id: 1, title: 'Kingmakers RP Grand Opening!', date: '2026-11-05', author: 'Management' },
            { id: 2, title: 'LSPD & EMS Academy Applications Open', date: '2026-11-05', author: 'LSPD Command' },
            { id: 3, title: 'Turf Wars Active in Ganton & Idlewood', date: '2026-11-05', author: 'Staff' }
        ]
    });
});

app.get('/api/leaderboards', (req, res) => {
    const db = getDB();
    if (!db) return res.json({ richest: [], topLevel: [] });
    try {
        const richest = db.prepare('SELECT id, name, money + bank AS total, level FROM accounts ORDER BY total DESC LIMIT 10').all();
        const topLevel = db.prepare('SELECT id, name, level, exp, playmin FROM accounts ORDER BY level DESC, exp DESC LIMIT 10').all();
        db.close();
        res.json({ richest, topLevel });
    } catch (e) {
        res.json({ richest: [], topLevel: [] });
    }
});

app.get('/api/turfs', (req, res) => {
    const db = getDB();
    if (!db) return res.json([]);
    try {
        const rows = db.prepare('SELECT id, faction FROM turfs').all();
        const factions = ['Civilian', 'LSPD', 'EMS', 'Grove Street Families', 'Ballas'];
        const names = ['Ganton', 'Idlewood', 'Glen Park', 'Jefferson'];
        const list = rows.map(r => ({
            id: r.id,
            name: names[r.id] || `Turf #${r.id}`,
            factionId: r.faction,
            factionName: factions[r.faction] || 'Unknown'
        }));
        db.close();
        res.json(list);
    } catch (e) {
        res.json([]);
    }
});

// Fallback to SPA dashboard
app.get('*', (req, res) => {
    res.sendFile(path.join(__dirname, 'public/index.html'));
});

app.listen(PORT, () => {
    console.log(`[KM Backend] API & Web Dashboard running on http://0.0.0.0:${PORT}`);
    console.log(`[KM Backend] Querying SA-MP server at ${SAMP_HOST}:${SAMP_PORT}`);
});
