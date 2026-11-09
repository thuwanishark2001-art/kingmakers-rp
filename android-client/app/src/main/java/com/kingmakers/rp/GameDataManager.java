package com.kingmakers.rp;

import android.os.Environment;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FilterInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * Handles the GTA: San Andreas engine data the client runs on.
 *
 * Kingmakers RP does not ship or host Rockstar's game data. Each player must own
 * GTA: San Andreas for Android; this class imports the data from that installation
 * (its OBB archives), removes the placeholder files earlier launcher builds wrote,
 * and checks the data is real before the engine is started.
 */
public final class GameDataManager {

    public static final String GTASA_PACKAGE = "com.rockstargames.gtasa";

    /** Top-level folders of our own content pack / APK assets. Nothing else is ever written by us. */
    public static final List<String> OWN_CONTENT_DIRS = Arrays.asList(
            "SAMP/", "gui/", "fonts/", "kingmakers/", "audio/streams/");

    private static final long MIN_GTA3_IMG_BYTES = 50L * 1024 * 1024;
    private static final long MIN_PED_IFP_BYTES = 100L * 1024;
    private static final int MIN_GTA3_IMG_ENTRIES = 1000;

    private GameDataManager() {}

    public interface ProgressListener {
        void onProgress(long bytesRead, long totalBytes, String currentEntry);
    }

    // ------------------------------------------------------------------
    // Locating the player's own GTA SA installation
    // ------------------------------------------------------------------

    /** OBB archives of an installed GTA SA, main.* before patch.*. Empty if none are readable. */
    public static List<File> findInstalledObbs() {
        List<File> result = new ArrayList<>();
        File obbDir = new File(Environment.getExternalStorageDirectory(), "Android/obb/" + GTASA_PACKAGE);
        File[] files = obbDir.listFiles();
        if (files == null) return result;
        for (File f : files) {
            String n = f.getName().toLowerCase();
            if (f.isFile() && n.endsWith(".obb") && f.length() > 0 && f.canRead()) result.add(f);
        }
        result.sort((a, b) -> Integer.compare(obbOrder(a.getName()), obbOrder(b.getName())));
        return result;
    }

    /** Folder inside our own storage where an OBB can be dropped (e.g. by adb / Termux) for automatic import. */
    public static File dropInDir(File storageDir) {
        return new File(storageDir, "import");
    }

    /** OBB/zip archives placed in {@link #dropInDir}, main.* before patch.*. Always readable by this app. */
    public static List<File> findDropInObbs(File storageDir) {
        List<File> result = new ArrayList<>();
        File[] files = dropInDir(storageDir).listFiles();
        if (files == null) return result;
        for (File f : files) {
            String n = f.getName().toLowerCase();
            if (f.isFile() && (n.endsWith(".obb") || n.endsWith(".zip")) && f.length() > 0) result.add(f);
        }
        result.sort((a, b) -> Integer.compare(obbOrder(a.getName()), obbOrder(b.getName())));
        return result;
    }

    /** main.* archives first, patch.* archives after, so patches override base files. */
    public static int obbOrder(String name) {
        String n = name == null ? "" : name.toLowerCase();
        if (n.startsWith("main.")) return 0;
        if (n.startsWith("patch.")) return 2;
        return 1;
    }

    // ------------------------------------------------------------------
    // Import
    // ------------------------------------------------------------------

    /**
     * Extracts a GTA SA OBB (a zip archive) into the game storage dir.
     * Our own content folders are never overwritten. Returns the number of files written.
     */
    public static int importArchive(InputStream raw, long totalBytes, File targetDir,
                                    ProgressListener listener) throws IOException {
        CountingInputStream counting = new CountingInputStream(new BufferedInputStream(raw, 1 << 16));
        String canonicalTarget = targetDir.getCanonicalPath() + File.separator;
        byte[] buffer = new byte[1 << 16];
        int written = 0;
        long lastReport = 0;

        try (ZipInputStream zis = new ZipInputStream(counting)) {
            ZipEntry entry;
            while ((entry = zis.getNextEntry()) != null) {
                String name = entry.getName().replace('\\', '/');
                while (name.startsWith("/")) name = name.substring(1);
                if (name.isEmpty() || isOwnContent(name)) {
                    zis.closeEntry();
                    continue;
                }

                File outFile = resolveForWrite(targetDir, name);
                // Zip-slip guard
                if (!outFile.getCanonicalPath().startsWith(canonicalTarget)) {
                    zis.closeEntry();
                    continue;
                }

                if (entry.isDirectory()) {
                    outFile.mkdirs();
                } else {
                    File parent = outFile.getParentFile();
                    if (parent != null && !parent.exists()) parent.mkdirs();
                    try (FileOutputStream fos = new FileOutputStream(outFile)) {
                        int len;
                        while ((len = zis.read(buffer)) > 0) {
                            fos.write(buffer, 0, len);
                            long now = System.currentTimeMillis();
                            if (listener != null && now - lastReport > 300) {
                                lastReport = now;
                                listener.onProgress(counting.count, totalBytes, name);
                            }
                        }
                    }
                    written++;
                }
                zis.closeEntry();
            }
        }
        if (listener != null) listener.onProgress(totalBytes, totalBytes, "");
        return written;
    }

    /**
     * Maps an archive path onto the target dir, reusing any existing file or folder whose
     * name differs only in case, so "DATA/GTA.DAT" and "data/gta.dat" end up in one place.
     */
    static File resolveForWrite(File base, String rel) {
        File cur = base;
        for (String part : rel.split("/")) {
            if (part.isEmpty() || part.equals(".")) continue;
            File exact = new File(cur, part);
            if (!exact.exists() && !part.equals("..")) {
                File[] children = cur.listFiles();
                if (children != null) {
                    for (File c : children) {
                        if (c.getName().equalsIgnoreCase(part)) { exact = c; break; }
                    }
                }
            }
            cur = exact;
        }
        return cur;
    }

    public static boolean isOwnContent(String relPath) {
        String p = relPath.replace('\\', '/');
        for (String prefix : OWN_CONTENT_DIRS) {
            if (p.regionMatches(true, 0, prefix, 0, prefix.length())) return true;
        }
        return false;
    }

    // ------------------------------------------------------------------
    // Cleanup of placeholder files written by earlier launcher builds
    // ------------------------------------------------------------------

    /** Deletes the fabricated stand-in files older builds generated. Returns how many were removed. */
    public static int removePlaceholderFiles(File dir) {
        int removed = 0;
        String[] textStubs = {
                "SAMP/gta.dat", "SAMP/default.dat", "SAMP/handling.cfg",
                "data/gta.dat", "data/default.dat", "data/handling.cfg", "data/carcols.dat",
                "data/timecyc.dat", "data/surface.dat",
                "texdb/gta3/gta3.txt", "texdb/menu/menu.txt", "texdb/txd/txd.txt",
                "texdb/mobile/mobile.txt", "audio/CONFIG/AudioEvents.txt"
        };
        for (String rel : textStubs) {
            File f = resolve(dir, rel);
            if (f != null && isPlaceholderText(f) && f.delete()) removed++;
        }

        File pedIfp = resolve(dir, "anim/ped.ifp");
        if (pedIfp != null && pedIfp.length() <= 64 && pedIfp.delete()) removed++;

        for (String img : new String[] {"models/gta3.img", "models/gta_int.img", "models/player.img"}) {
            File f = resolve(dir, img);
            if (f != null && isZeroFilledImg(f) && f.delete()) removed++;
        }
        return removed;
    }

    private static boolean isPlaceholderText(File f) {
        if (!f.isFile() || f.length() > 64 * 1024) return false;
        try (BufferedReader r = new BufferedReader(new FileReader(f))) {
            String first = r.readLine();
            if (first == null) return true;
            first = first.trim();
            return first.startsWith("# Kingmakers RP") || first.startsWith("; Kingmakers RP")
                    || first.startsWith("# Texture Index for");
        } catch (IOException e) {
            return false;
        }
    }

    // ------------------------------------------------------------------
    // Validation
    // ------------------------------------------------------------------

    public static final class Result {
        public final List<String> problems = new ArrayList<>();
        public boolean isValid() { return problems.isEmpty(); }
        public String describe() {
            StringBuilder sb = new StringBuilder();
            for (String p : problems) sb.append("• ").append(p).append('\n');
            return sb.toString().trim();
        }
    }

    /** Checks the engine data is a real GTA SA installation and not missing or placeholder data. */
    public static Result validate(File dir) {
        Result r = new Result();

        File gta3 = resolve(dir, "models/gta3.img");
        if (gta3 == null) {
            r.problems.add("models/gta3.img is missing");
        } else {
            String err = checkImg(gta3, MIN_GTA3_IMG_BYTES, MIN_GTA3_IMG_ENTRIES);
            if (err != null) r.problems.add("models/gta3.img " + err);
        }

        for (String img : new String[] {"models/gta_int.img", "models/player.img"}) {
            File f = resolve(dir, img);
            if (f == null) {
                r.problems.add(img + " is missing");
            } else {
                String err = checkImg(f, 1, 1);
                if (err != null) r.problems.add(img + " " + err);
            }
        }

        File pedIfp = resolve(dir, "anim/ped.ifp");
        if (pedIfp == null) {
            r.problems.add("anim/ped.ifp is missing");
        } else if (pedIfp.length() < MIN_PED_IFP_BYTES) {
            r.problems.add("anim/ped.ifp is a placeholder (" + pedIfp.length() + " bytes, contains no animations)");
        }

        for (String dat : new String[] {"data/gta.dat", "data/default.dat"}) {
            File f = resolve(dir, dat);
            if (f == null) r.problems.add(dat + " is missing");
            else if (isPlaceholderText(f)) r.problems.add(dat + " is a placeholder, not the game's file");
        }
        return r;
    }

    /** Null if the IMG v2 archive looks real, otherwise a short reason. */
    private static String checkImg(File f, long minBytes, int minEntries) {
        if (f.length() < minBytes) return "is too small (" + f.length() + " bytes)";
        try (RandomAccessFile raf = new RandomAccessFile(f, "r")) {
            byte[] magic = new byte[4];
            raf.readFully(magic);
            if (magic[0] != 'V' || magic[1] != 'E' || magic[2] != 'R' || magic[3] != '2') {
                return "is not a GTA IMG archive";
            }
            int count = readIntLE(raf);
            if (count < minEntries) return "has only " + count + " entries";
            if (isZeroFilledImg(f)) return "is a placeholder (model data is all zeros)";
        } catch (IOException e) {
            return "cannot be read (" + e.getMessage() + ")";
        }
        return null;
    }

    /** True for an IMG v2 whose first sampled entries contain only zero bytes (the old fake archives). */
    private static boolean isZeroFilledImg(File f) {
        try (RandomAccessFile raf = new RandomAccessFile(f, "r")) {
            if (raf.length() < 8) return false;
            byte[] magic = new byte[4];
            raf.readFully(magic);
            if (magic[0] != 'V' || magic[1] != 'E' || magic[2] != 'R' || magic[3] != '2') return false;
            int count = readIntLE(raf);
            if (count <= 0) return true;

            int samples = Math.min(count, 8);
            byte[] block = new byte[2048];
            for (int i = 0; i < samples; i++) {
                int idx = (int) ((long) i * count / samples);
                raf.seek(8L + idx * 32L);
                long sector = readIntLE(raf) & 0xFFFFFFFFL;
                long offset = sector * 2048L;
                if (offset + block.length > raf.length()) continue;
                raf.seek(offset);
                raf.readFully(block);
                for (byte b : block) if (b != 0) return false;
            }
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    private static int readIntLE(RandomAccessFile raf) throws IOException {
        int b0 = raf.read(), b1 = raf.read(), b2 = raf.read(), b3 = raf.read();
        if ((b0 | b1 | b2 | b3) < 0) throw new IOException("unexpected end of file");
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }

    /** Resolves a relative path case-insensitively (OBB entries and engine paths differ in case). */
    public static File resolve(File base, String rel) {
        File cur = base;
        for (String part : rel.split("/")) {
            File exact = new File(cur, part);
            if (exact.exists()) { cur = exact; continue; }
            File[] children = cur.listFiles();
            File match = null;
            if (children != null) {
                for (File c : children) {
                    if (c.getName().equalsIgnoreCase(part)) { match = c; break; }
                }
            }
            if (match == null) return null;
            cur = match;
        }
        return cur;
    }

    private static final class CountingInputStream extends FilterInputStream {
        long count;
        CountingInputStream(InputStream in) { super(in); }
        @Override public int read() throws IOException {
            int b = super.read();
            if (b >= 0) count++;
            return b;
        }
        @Override public int read(byte[] b, int off, int len) throws IOException {
            int n = super.read(b, off, len);
            if (n > 0) count += n;
            return n;
        }
        @Override public long skip(long n) throws IOException {
            long s = super.skip(n);
            if (s > 0) count += s;
            return s;
        }
    }
}
