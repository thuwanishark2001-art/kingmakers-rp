'use strict';
/**
 * Kingmakers RP - game API for the Unity client and game server (/api/v2).
 *
 * Players:
 *   POST /auth/register  {username, password, characterName} -> {token, character}
 *   POST /auth/login     {username, password}                -> {token, character}
 *   GET  /game/config                                        -> {host, port, version}
 *
 * Game server only (header "x-server-key: <GAME_SERVER_KEY>"):
 *   POST /server/verify          {token}           -> {character}
 *   PUT  /server/characters/:id  {cash, bank, ...} -> {ok}
 *
 * Data lives in one JSON file written atomically (tmp file + rename). That is
 * enough for the first playable build; move to SQLite/Postgres before launch.
 */

const express = require('express');
const crypto = require('crypto');
const fs = require('fs');
const path = require('path');

const TOKEN_TTL_MS = 12 * 60 * 60 * 1000;
const START_CASH = 5000;
const START_BANK = 20000;
const MAX_MONEY = 1e12;

const USERNAME_RE = /^[A-Za-z0-9_]{3,20}$/;
const CHARACTER_RE = /^[A-Z][a-z]{1,15}_[A-Z][a-z]{1,15}$/;

class JsonStore {
    constructor(file) {
        this.file = file;
        this.data = { nextAccountId: 1, accounts: {}, characters: {} };
        if (fs.existsSync(file)) {
            this.data = JSON.parse(fs.readFileSync(file, 'utf8'));
        }
    }

    save() {
        fs.mkdirSync(path.dirname(this.file), { recursive: true });
        const tmp = `${this.file}.${process.pid}.tmp`;
        fs.writeFileSync(tmp, JSON.stringify(this.data));
        fs.renameSync(tmp, this.file);
    }
}

function hashPassword(password, salt) {
    return crypto.scryptSync(password, salt, 64).toString('hex');
}

function safeEqual(a, b) {
    const ab = Buffer.from(String(a));
    const bb = Buffer.from(String(b));
    return ab.length === bb.length && crypto.timingSafeEqual(ab, bb);
}

function publicCharacter(c) {
    return {
        id: c.id, name: c.name, cash: c.cash, bank: c.bank,
        level: c.level, exp: c.exp, playMinutes: c.playMinutes,
        hasPosition: !!c.position, position: c.position || { x: 0, y: 0, z: 0 }, rotY: c.rotY,
    };
}

function createGameApi(options = {}) {
    const store = new JsonStore(options.dataFile || path.join(__dirname, 'data', 'game.json'));
    const tokenSecret = options.tokenSecret || crypto.randomBytes(32).toString('hex');
    const serverKey = options.serverKey || '';
    const now = options.now || (() => Date.now());
    const gameHost = options.gameHost || '127.0.0.1';
    const gamePort = options.gamePort || 7777;

    const failedLogins = new Map(); // ip -> {count, until}

    function signToken(payload) {
        const body = Buffer.from(JSON.stringify(payload)).toString('base64url');
        const mac = crypto.createHmac('sha256', tokenSecret).update(body).digest('base64url');
        return `${body}.${mac}`;
    }

    function readToken(token) {
        if (typeof token !== 'string' || !token.includes('.')) return null;
        const [body, mac] = token.split('.');
        const expected = crypto.createHmac('sha256', tokenSecret).update(body).digest('base64url');
        if (!safeEqual(mac, expected)) return null;
        let payload;
        try { payload = JSON.parse(Buffer.from(body, 'base64url').toString('utf8')); } catch { return null; }
        if (!payload || typeof payload.exp !== 'number' || payload.exp < now()) return null;
        return payload;
    }

    function issue(character) {
        return signToken({ cid: character.id, exp: now() + TOKEN_TTL_MS });
    }

    function tooManyFailures(ip) {
        const entry = failedLogins.get(ip);
        return entry && entry.count >= 10 && entry.until > now();
    }

    function noteFailure(ip) {
        const entry = failedLogins.get(ip);
        if (!entry || entry.until < now()) failedLogins.set(ip, { count: 1, until: now() + 10 * 60 * 1000 });
        else entry.count++;
    }

    function requireServer(req, res, next) {
        if (!serverKey) return res.status(503).json({ error: 'GAME_SERVER_KEY is not configured' });
        if (!safeEqual(req.get('x-server-key') || '', serverKey)) return res.status(401).json({ error: 'bad server key' });
        next();
    }

    const router = express.Router();
    router.use(express.json({ limit: '16kb' }));

    router.post('/auth/register', (req, res) => {
        const { username, password, characterName } = req.body || {};
        if (typeof username !== 'string' || !USERNAME_RE.test(username)) {
            return res.status(400).json({ error: 'Username: 3-20 letters, digits or _' });
        }
        if (typeof password !== 'string' || password.length < 6 || password.length > 72) {
            return res.status(400).json({ error: 'Password: 6-72 characters' });
        }
        if (typeof characterName !== 'string' || !CHARACTER_RE.test(characterName)) {
            return res.status(400).json({ error: 'Character name must look like Firstname_Lastname' });
        }
        const key = username.toLowerCase();
        if (store.data.accounts[key]) return res.status(409).json({ error: 'Username is taken' });
        const nameTaken = Object.values(store.data.characters)
            .some(c => c.name.toLowerCase() === characterName.toLowerCase());
        if (nameTaken) return res.status(409).json({ error: 'Character name is taken' });

        const id = store.data.nextAccountId++;
        const salt = crypto.randomBytes(16).toString('hex');
        store.data.accounts[key] = { id, username, salt, hash: hashPassword(password, salt), createdAt: now() };
        const character = {
            id, accountId: id, name: characterName, cash: START_CASH, bank: START_BANK,
            level: 1, exp: 0, playMinutes: 0, position: null, rotY: 0, updatedAt: now(),
        };
        store.data.characters[id] = character;
        store.save();
        res.status(201).json({ token: issue(character), character: publicCharacter(character) });
    });

    router.post('/auth/login', (req, res) => {
        const ip = req.ip || 'unknown';
        if (tooManyFailures(ip)) return res.status(429).json({ error: 'Too many attempts, try again later' });
        const { username, password } = req.body || {};
        const account = typeof username === 'string' ? store.data.accounts[username.toLowerCase()] : null;
        if (!account || typeof password !== 'string' || !safeEqual(hashPassword(password, account.salt), account.hash)) {
            noteFailure(ip);
            return res.status(401).json({ error: 'Wrong username or password' });
        }
        failedLogins.delete(ip);
        const character = store.data.characters[account.id];
        res.json({ token: issue(character), character: publicCharacter(character) });
    });

    router.get('/game/config', (req, res) => {
        res.json({ host: gameHost, port: gamePort, version: '0.1.0' });
    });

    router.post('/server/verify', requireServer, (req, res) => {
        const payload = readToken((req.body || {}).token);
        if (!payload) return res.status(401).json({ error: 'invalid or expired token' });
        const character = store.data.characters[payload.cid];
        if (!character) return res.status(404).json({ error: 'character not found' });
        res.json({ character: publicCharacter(character) });
    });

    router.put('/server/characters/:id', requireServer, (req, res) => {
        const character = store.data.characters[req.params.id];
        if (!character) return res.status(404).json({ error: 'character not found' });
        const b = req.body || {};
        const money = v => Number.isInteger(v) && v >= 0 && v <= MAX_MONEY;
        const finite = v => typeof v === 'number' && Number.isFinite(v);
        if (!money(b.cash) || !money(b.bank)) return res.status(400).json({ error: 'cash and bank must be whole numbers >= 0' });
        character.cash = b.cash;
        character.bank = b.bank;
        if (Number.isInteger(b.level) && b.level >= 1) character.level = b.level;
        if (Number.isInteger(b.exp) && b.exp >= 0) character.exp = b.exp;
        if (Number.isInteger(b.playMinutes) && b.playMinutes >= 0) character.playMinutes = b.playMinutes;
        if (b.position && finite(b.position.x) && finite(b.position.y) && finite(b.position.z)) {
            character.position = { x: b.position.x, y: b.position.y, z: b.position.z };
        }
        if (finite(b.rotY)) character.rotY = b.rotY;
        character.updatedAt = now();
        store.save();
        res.json({ ok: true });
    });

    return router;
}

module.exports = { createGameApi };
