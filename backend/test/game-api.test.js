'use strict';
const test = require('node:test');
const assert = require('node:assert');
const fs = require('fs');
const os = require('os');
const path = require('path');
const Module = require('module');

// Use real Express when installed; otherwise a minimal stand-in (see support/mini-express.js).
let express;
try {
    express = require('express');
} catch {
    const shim = require.resolve('./support/mini-express');
    const resolve = Module._resolveFilename;
    Module._resolveFilename = function (request, ...rest) {
        return request === 'express' ? shim : resolve.call(this, request, ...rest);
    };
    express = require(shim);
}
const { createGameApi } = require('../game-api');

async function start(opts = {}) {
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'km-game-api-'));
    const dataFile = path.join(dir, 'game.json');
    const app = express();
    app.use('/api/v2', createGameApi({ dataFile, tokenSecret: 'secret', serverKey: 'srv', ...opts }));
    const server = await new Promise(r => { const s = app.listen(0, () => r(s)); });
    const base = `http://127.0.0.1:${server.address().port}/api/v2`;
    const call = async (method, url, body, headers = {}) => {
        const res = await fetch(base + url, {
            method, headers: { 'content-type': 'application/json', ...headers },
            body: body === undefined ? undefined : JSON.stringify(body),
        });
        return { status: res.status, body: await res.json() };
    };
    return { call, dataFile, close: () => server.close() };
}

const player = { username: 'alex_99', password: 'hunter22', characterName: 'Alex_Cross' };

test('register, login, verify and save a character', async () => {
    const api = await start();
    try {
        const reg = await api.call('POST', '/auth/register', player);
        assert.strictEqual(reg.status, 201);
        assert.strictEqual(reg.body.character.name, 'Alex_Cross');
        assert.strictEqual(reg.body.character.cash, 5000);

        const login = await api.call('POST', '/auth/login', { username: 'ALEX_99', password: 'hunter22' });
        assert.strictEqual(login.status, 200);

        const verify = await api.call('POST', '/server/verify', { token: login.body.token }, { 'x-server-key': 'srv' });
        assert.strictEqual(verify.status, 200);
        const id = verify.body.character.id;

        const save = await api.call('PUT', `/server/characters/${id}`,
            { cash: 1234, bank: 99, position: { x: 1, y: 2, z: 3 }, rotY: 90 }, { 'x-server-key': 'srv' });
        assert.strictEqual(save.status, 200);

        const again = await api.call('POST', '/server/verify', { token: login.body.token }, { 'x-server-key': 'srv' });
        assert.deepStrictEqual(again.body.character.position, { x: 1, y: 2, z: 3 });
        assert.strictEqual(again.body.character.cash, 1234);
        assert.strictEqual(JSON.parse(fs.readFileSync(api.dataFile, 'utf8')).characters[id].bank, 99);
    } finally { api.close(); }
});

test('rejects bad input, duplicates and wrong passwords', async () => {
    const api = await start();
    try {
        assert.strictEqual((await api.call('POST', '/auth/register', { ...player, characterName: 'alex' })).status, 400);
        assert.strictEqual((await api.call('POST', '/auth/register', { ...player, password: '123' })).status, 400);
        assert.strictEqual((await api.call('POST', '/auth/register', player)).status, 201);
        assert.strictEqual((await api.call('POST', '/auth/register', player)).status, 409);
        assert.strictEqual((await api.call('POST', '/auth/register', { ...player, username: 'other' })).status, 409);
        assert.strictEqual((await api.call('POST', '/auth/login', { username: 'alex_99', password: 'nope!!' })).status, 401);
    } finally { api.close(); }
});

test('server endpoints need the server key and a valid token', async () => {
    const api = await start();
    try {
        const reg = await api.call('POST', '/auth/register', player);
        const id = reg.body.character.id;
        assert.strictEqual((await api.call('POST', '/server/verify', { token: reg.body.token })).status, 401);
        assert.strictEqual((await api.call('POST', '/server/verify', { token: reg.body.token }, { 'x-server-key': 'bad' })).status, 401);
        const forged = reg.body.token.replace(/.$/, c => (c === 'A' ? 'B' : 'A'));
        assert.strictEqual((await api.call('POST', '/server/verify', { token: forged }, { 'x-server-key': 'srv' })).status, 401);
        assert.strictEqual((await api.call('PUT', `/server/characters/${id}`, { cash: -5, bank: 0 }, { 'x-server-key': 'srv' })).status, 400);
    } finally { api.close(); }
});

test('expired tokens are refused', async () => {
    let t = 1_000_000;
    const api = await start({ now: () => t });
    try {
        const reg = await api.call('POST', '/auth/register', player);
        t += 13 * 60 * 60 * 1000;
        assert.strictEqual((await api.call('POST', '/server/verify', { token: reg.body.token }, { 'x-server-key': 'srv' })).status, 401);
    } finally { api.close(); }
});

test('server endpoints are off when no server key is configured', async () => {
    const api = await start({ serverKey: '' });
    try {
        assert.strictEqual((await api.call('POST', '/server/verify', { token: 'x' }, { 'x-server-key': '' })).status, 503);
    } finally { api.close(); }
});

test('login is rate limited after repeated failures', async () => {
    const api = await start();
    try {
        await api.call('POST', '/auth/register', player);
        for (let i = 0; i < 10; i++) await api.call('POST', '/auth/login', { username: 'alex_99', password: 'wrong!!' });
        assert.strictEqual((await api.call('POST', '/auth/login', player)).status, 429);
    } finally { api.close(); }
});
