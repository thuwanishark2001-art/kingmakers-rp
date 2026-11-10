package com.kingmakers.rp;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayDeque;
import java.util.LinkedHashSet;

/**
 * Collects what a tester needs to send after a failed run: the files the engine asked
 * for and could not find, the end of the client log, and the crash log. The game folder
 * (Android/data/...) is not reachable from a file manager on recent Android, so the
 * launcher copies this summary to the clipboard instead.
 */
public final class DebugLog {

    private static final int TAIL_LINES = 120;
    private static final int MAX_MISSING = 200;
    private static final int MAX_CHARS = 60000;

    private DebugLog() {}

    private static File logFile(Context ctx)   { return new File(DownloaderActivity.getGameStorageDir(ctx), "log.txt"); }
    private static File crashFile(Context ctx) { return new File(DownloaderActivity.getGameStorageDir(ctx), "crash_log.txt"); }

    public static boolean exists(Context ctx) {
        return logFile(ctx).length() > 0 || crashFile(ctx).length() > 0;
    }

    /** Starts a fresh log for the run that is about to begin (the client appends to log.txt). */
    public static void startNewRun(Context ctx) {
        logFile(ctx).delete();
        crashFile(ctx).delete();
    }

    public static String summary(Context ctx) {
        StringBuilder out = new StringBuilder();
        out.append("Kingmakers RP debug log\n");

        LinkedHashSet<String> missing = new LinkedHashSet<>();
        ArrayDeque<String> tail = new ArrayDeque<>();
        int opened = 0, total = 0;
        File log = logFile(ctx);
        if (log.exists()) {
            try (BufferedReader r = new BufferedReader(new FileReader(log))) {
                String line;
                while ((line = r.readLine()) != null) {
                    total++;
                    if (line.startsWith("[file] MISS ")) {
                        if (missing.size() < MAX_MISSING) missing.add(line.substring(12));
                    } else if (line.startsWith("[file] OK")) {
                        opened++;
                    }
                    tail.addLast(line);
                    if (tail.size() > TAIL_LINES) tail.removeFirst();
                }
            } catch (Exception e) {
                out.append("(could not read log.txt: ").append(e.getMessage()).append(")\n");
            }
        } else {
            out.append("(no log.txt: the game has not been started since the last copy)\n");
        }

        out.append("log lines: ").append(total).append(", files opened: ").append(opened)
           .append(", files missing: ").append(missing.size()).append("\n\n== MISSING FILES (in request order) ==\n");
        for (String m : missing) out.append(m).append('\n');

        out.append("\n== LAST ").append(tail.size()).append(" LOG LINES ==\n");
        for (String t : tail) out.append(t).append('\n');

        File crash = crashFile(ctx);
        if (crash.length() > 0) {
            out.append("\n== CRASH LOG ==\n");
            try (BufferedReader r = new BufferedReader(new FileReader(crash))) {
                String line;
                int n = 0;
                while ((line = r.readLine()) != null && n++ < 150) out.append(line).append('\n');
            } catch (Exception ignored) {}
        }

        if (out.length() > MAX_CHARS) {
            // Keep the head (missing files) and the very end (where it died)
            String s = out.toString();
            return s.substring(0, MAX_CHARS / 2) + "\n...[cut]...\n" + s.substring(s.length() - MAX_CHARS / 2);
        }
        return out.toString();
    }

    /** Copies the summary to the clipboard; returns its length in characters. */
    public static int copyToClipboard(Context ctx) {
        String text = summary(ctx);
        ClipboardManager cm = (ClipboardManager) ctx.getSystemService(Context.CLIPBOARD_SERVICE);
        cm.setPrimaryClip(ClipData.newPlainText("Kingmakers RP debug log", text));
        return text.length();
    }
}
