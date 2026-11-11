'use strict';
/**
 * Just enough of Express to run game-api.js in tests when node_modules is not
 * installed (express.json, Router with get/post/put/use, app.use(prefix), listen).
 * With the real package installed, the tests use Express itself.
 */
const http = require('http');

function compile(path) {
    const keys = [];
    const re = new RegExp('^' + path.replace(/:([A-Za-z_]+)/g, (_, k) => { keys.push(k); return '([^/]+)'; }) + '/?$');
    return { re, keys };
}

function Router() {
    const stack = [];
    const router = (req, res, next) => {
        let i = 0;
        const step = () => {
            const layer = stack[i++];
            if (!layer) return next();
            if (layer.method && layer.method !== req.method) return step();
            if (layer.match) {
                const m = layer.match.re.exec(req.path);
                if (!m) return step();
                req.params = {};
                layer.match.keys.forEach((k, n) => { req.params[k] = decodeURIComponent(m[n + 1]); });
            }
            let h = 0;
            const runHandler = () => {
                const fn = layer.handlers[h++];
                if (!fn) return step();
                fn(req, res, runHandler);
            };
            runHandler();
        };
        step();
    };
    router.use = (...handlers) => { stack.push({ handlers }); return router; };
    for (const method of ['get', 'post', 'put', 'delete']) {
        router[method] = (path, ...handlers) => {
            stack.push({ method: method.toUpperCase(), match: compile(path), handlers });
            return router;
        };
    }
    return router;
}

function json() {
    return (req, res, next) => {
        if (req.body !== undefined || req.method === 'GET') { req.body = req.body || {}; return next(); }
        let data = '';
        req.on('data', c => { data += c; });
        req.on('end', () => {
            try { req.body = data ? JSON.parse(data) : {}; } catch { req.body = {}; }
            next();
        });
    };
}

function express() {
    const mounts = [];
    const app = (req, res) => {
        const url = new URL(req.url, 'http://x');
        req.ip = req.socket.remoteAddress;
        req.get = name => req.headers[name.toLowerCase()];
        res.status = code => { res.statusCode = code; return res; };
        res.json = obj => { res.setHeader('content-type', 'application/json'); res.end(JSON.stringify(obj)); };
        for (const { prefix, handler } of mounts) {
            if (url.pathname.startsWith(prefix)) {
                req.path = url.pathname.slice(prefix.length) || '/';
                return handler(req, res, () => res.status(404).json({ error: 'not found' }));
            }
        }
        res.status(404).json({ error: 'not found' });
    };
    app.use = (prefix, handler) => mounts.push({ prefix, handler });
    app.listen = (port, cb) => http.createServer(app).listen(port, cb);
    return app;
}

express.Router = Router;
express.json = json;
module.exports = express;
