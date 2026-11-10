package com.kingmakers.rp;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.MessageDigest;

/**
 * Downloads a large package to a file, resuming a partial download when the
 * server supports HTTP ranges, and verifies its SHA-256 before it is used.
 */
public final class PackDownloader {

    private PackDownloader() {}

    public interface Progress {
        void onProgress(long done, long total);
    }

    /**
     * Downloads url into target. A partial target from an earlier attempt is continued.
     * Throws IOException on network errors (the partial file is kept for the next try)
     * and on a checksum mismatch (the file is deleted, since resuming it cannot help).
     */
    public static void download(String url, File target, String expectedSha256, Progress progress)
            throws IOException {
        File parent = target.getParentFile();
        if (parent != null && !parent.exists()) parent.mkdirs();

        long have = target.exists() ? target.length() : 0;
        HttpURLConnection conn = open(url, have);
        try {
            int code = conn.getResponseCode();
            if (code == 416 && have > 0) {
                // Nothing left to fetch: the file is already whole (or stale); the checksum decides
                conn.disconnect();
            } else {
                if (code == HttpURLConnection.HTTP_OK) {
                    have = 0;   // server ignored the range: start over
                } else if (code != HttpURLConnection.HTTP_PARTIAL) {
                    throw new IOException("Download failed with HTTP " + code);
                }
                long remaining = conn.getContentLengthLong();
                long total = remaining >= 0 ? have + remaining : -1;

                try (InputStream in = conn.getInputStream();
                     RandomAccessFile out = new RandomAccessFile(target, "rw")) {
                    out.setLength(have);
                    out.seek(have);
                    byte[] buffer = new byte[1 << 16];
                    long done = have, lastReport = 0;
                    int n;
                    while ((n = in.read(buffer)) != -1) {
                        out.write(buffer, 0, n);
                        done += n;
                        long now = System.currentTimeMillis();
                        if (progress != null && now - lastReport > 300) {
                            lastReport = now;
                            progress.onProgress(done, total);
                        }
                    }
                    if (total >= 0 && done != total) {
                        throw new IOException("Connection closed early (" + done + " of " + total + " bytes)");
                    }
                }
            }
        } finally {
            conn.disconnect();
        }

        if (expectedSha256 != null && !expectedSha256.isEmpty()) {
            String actual = sha256(target);
            if (!expectedSha256.trim().equalsIgnoreCase(actual)) {
                target.delete();
                throw new IOException("Downloaded file is damaged (checksum mismatch). Please try again.");
            }
        }
    }

    private static HttpURLConnection open(String url, long from) throws IOException {
        HttpURLConnection conn = (HttpURLConnection) new URL(url).openConnection();
        conn.setConnectTimeout(15000);
        conn.setReadTimeout(30000);
        // Byte offsets must refer to the stored file, not a compressed transfer
        conn.setRequestProperty("Accept-Encoding", "identity");
        if (from > 0) conn.setRequestProperty("Range", "bytes=" + from + "-");
        return conn;
    }

    public static String sha256(File f) throws IOException {
        try (InputStream in = new FileInputStream(f)) {
            MessageDigest md = MessageDigest.getInstance("SHA-256");
            byte[] buffer = new byte[1 << 16];
            int n;
            while ((n = in.read(buffer)) != -1) md.update(buffer, 0, n);
            StringBuilder hex = new StringBuilder();
            for (byte b : md.digest()) hex.append(String.format("%02x", b));
            return hex.toString();
        } catch (java.security.NoSuchAlgorithmException e) {
            throw new IOException(e);
        }
    }
}
