package com.kingmakers.rp;

import com.russia.game.R;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import org.json.JSONObject;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;
import okhttp3.ResponseBody;

/**
 * Game setup before the first launch:
 *  1. removes placeholder files written by older launcher builds,
 *  2. imports the player's own GTA: San Andreas data (installed OBB, or a file they pick),
 *  3. validates that data,
 *  4. downloads the Kingmakers RP content pack (our own SAMP/GUI/font/config files only),
 *  5. launches the multiplayer client.
 */
public class DownloaderActivity extends AppCompatActivity {

    private static final int REQ_STORAGE = 4101;
    private static final String CONTENT_MARKER = "kingmakers/.content_pack";

    private enum Step { CHECKING, NEED_GAME_DATA, IMPORT_FOUND, NEED_CONTENT, READY, BUSY }

    private ProgressBar progressBar;
    private TextView textTitle;
    private TextView textStatus;
    private Button btnAction;
    private Button btnPick;

    private String playerNick = "Marcus_King";
    private String serverIp = "139.180.185.57";
    private int serverPort = 7777;

    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final OkHttpClient httpClient = new OkHttpClient();
    private Step step = Step.CHECKING;
    private List<File> foundObbs = new ArrayList<>();
    private boolean storagePermissionAsked = false;

    private ActivityResultLauncher<String[]> pickObbLauncher;

    // ------------------------------------------------------------------
    // Storage location (shared with Samp.kt / LauncherActivity)
    // ------------------------------------------------------------------

    public static File getGameStorageDir(Context ctx) {
        File[] candidates = new File[] {
                new File("/storage/emulated/0/Android/data/com.kingmakers.rp/files"),
                new File("/storage/emulated/0/Android/data/com.russia.game/files"),
                ctx.getExternalFilesDir(null),
                ctx.getFilesDir()
        };

        for (File dir : candidates) {
            if (dir != null && dir.exists()) {
                if (new File(dir, "data/gta.dat").exists() ||
                    new File(dir, "SAMP").exists() ||
                    new File(dir, "models/gta3.img").exists()) {
                    return dir;
                }
            }
        }

        File dir = ctx.getExternalFilesDir(null);
        if (dir != null) {
            if (!dir.exists()) dir.mkdirs();
            return dir;
        }
        return ctx.getFilesDir();
    }

    /** True when real GTA SA data and the Kingmakers content are both in place. */
    public static boolean isReadyToPlay(Context ctx) {
        File dir = getGameStorageDir(ctx);
        return GameDataManager.validate(dir).isValid() && new File(dir, CONTENT_MARKER).exists();
    }

    // ------------------------------------------------------------------
    // Lifecycle
    // ------------------------------------------------------------------

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_downloader);

        progressBar = findViewById(R.id.downloadProgress);
        textTitle = findViewById(R.id.textDownloadTitle);
        textStatus = findViewById(R.id.textDownloadStatus);
        btnAction = findViewById(R.id.btnDownloadAction);
        btnPick = findViewById(R.id.btnPickGameData);

        if (getIntent() != null) {
            if (getIntent().hasExtra("nick")) playerNick = getIntent().getStringExtra("nick");
            if (getIntent().hasExtra("ip")) serverIp = getIntent().getStringExtra("ip");
            if (getIntent().hasExtra("port")) serverPort = getIntent().getIntExtra("port", 7777);
        }

        pickObbLauncher = registerForActivityResult(
                new ActivityResultContracts.OpenMultipleDocuments(),
                uris -> { if (uris != null && !uris.isEmpty()) importPickedFiles(uris); });

        btnAction.setOnClickListener(v -> onActionClicked());
        btnPick.setOnClickListener(v -> openObbPicker());

        checkState();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        executor.shutdownNow();
    }

    private void onActionClicked() {
        switch (step) {
            case NEED_GAME_DATA: openObbPicker(); break;
            case IMPORT_FOUND:   importInstalledObbs(); break;
            case NEED_CONTENT:   downloadContentPack(); break;
            case READY:          launchGame(); break;
            default: break;
        }
    }

    // ------------------------------------------------------------------
    // State check
    // ------------------------------------------------------------------

    private void checkState() {
        setBusy("CHECKING GAME FILES", "Checking game files...");
        executor.execute(() -> {
            File dir = getGameStorageDir(this);
            int removed = GameDataManager.removePlaceholderFiles(dir);
            extractBundledContent(this, dir);
            GameDataManager.Result result = GameDataManager.validate(dir);

            if (!result.isValid()) {
                List<File> obbs = GameDataManager.findInstalledObbs();
                runOnUiThread(() -> {
                    if (!obbs.isEmpty()) {
                        foundObbs = obbs;
                        showFoundInstallation(obbs);
                    } else if (needsStoragePermission()) {
                        storagePermissionAsked = true;
                        ActivityCompat.requestPermissions(this,
                                new String[] {Manifest.permission.READ_EXTERNAL_STORAGE}, REQ_STORAGE);
                    } else {
                        showNeedGameData(removed);
                    }
                });
                return;
            }

            boolean contentInstalled = new File(dir, CONTENT_MARKER).exists();
            runOnUiThread(() -> {
                if (!contentInstalled) showNeedContent();
                else showReady();
            });
        });
    }

    private boolean needsStoragePermission() {
        return !storagePermissionAsked
                && Build.VERSION.SDK_INT <= Build.VERSION_CODES.S_V2
                && ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
                        != PackageManager.PERMISSION_GRANTED;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQ_STORAGE) checkState();
    }

    // ------------------------------------------------------------------
    // UI states
    // ------------------------------------------------------------------

    private void setBusy(String title, String status) {
        step = Step.BUSY;
        textTitle.setText(title);
        textStatus.setText(status);
        btnAction.setEnabled(false);
        btnPick.setVisibility(View.GONE);
    }

    private void showFoundInstallation(List<File> obbs) {
        step = Step.IMPORT_FOUND;
        long total = 0;
        for (File f : obbs) total += f.length();
        textTitle.setText("GTA: SAN ANDREAS FOUND");
        textStatus.setText(String.format(
                "Found your GTA: San Andreas installation (%d file(s), %.0f MB).\n" +
                "Kingmakers RP runs on your copy of the game. Import its data to continue.",
                obbs.size(), total / (1024.0 * 1024.0)));
        progressBar.setProgress(0);
        btnAction.setEnabled(true);
        btnAction.setText("IMPORT GAME DATA");
        btnPick.setVisibility(View.VISIBLE);
    }

    private void showNeedGameData(int removedPlaceholders) {
        step = Step.NEED_GAME_DATA;
        textTitle.setText("GTA: SAN ANDREAS REQUIRED");
        String extra = removedPlaceholders > 0
                ? "\n\nRemoved " + removedPlaceholders + " placeholder file(s) left by an older version."
                : "";
        textStatus.setText(
                "Kingmakers RP runs on GTA: San Andreas for Android, which you need to own and install " +
                "from Google Play.\n\n" +
                "If it is installed but not detected, copy its OBB file from " +
                "Android/obb/" + GameDataManager.GTASA_PACKAGE + "/ to your Download folder " +
                "with a file manager, then select it here." + extra);
        progressBar.setProgress(0);
        btnAction.setEnabled(true);
        btnAction.setText("SELECT GTA SA OBB FILE");
        btnPick.setVisibility(View.GONE);
    }

    private void showNeedContent() {
        step = Step.NEED_CONTENT;
        textTitle.setText("KINGMAKERS RP CONTENT");
        textStatus.setText("Game data verified. Download the Kingmakers RP content pack to continue.");
        progressBar.setProgress(0);
        btnAction.setEnabled(true);
        btnAction.setText("DOWNLOAD");
        btnPick.setVisibility(View.GONE);
    }

    private void showReady() {
        step = Step.READY;
        textTitle.setText("READY");
        textStatus.setText("All game files verified. Ready to enter Los Santos.");
        progressBar.setProgress(100);
        btnAction.setEnabled(true);
        btnAction.setText("PLAY NOW");
        btnPick.setVisibility(View.GONE);
    }

    private void showValidationFailure(GameDataManager.Result result) {
        showNeedGameData(0);
        new AlertDialog.Builder(this)
                .setTitle("Game data incomplete")
                .setMessage("The imported files are not a complete GTA: San Andreas installation:\n\n"
                        + result.describe()
                        + "\n\nSelect the main OBB file (and the patch OBB, if your copy has one).")
                .setPositiveButton("OK", null)
                .show();
    }

    // ------------------------------------------------------------------
    // Importing the player's own GTA SA data
    // ------------------------------------------------------------------

    private void openObbPicker() {
        try {
            pickObbLauncher.launch(new String[] {"*/*"});
        } catch (Exception e) {
            Toast.makeText(this, "No file picker available: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }

    private void importInstalledObbs() {
        final List<File> obbs = new ArrayList<>(foundObbs);
        setBusy("IMPORTING GAME DATA", "Importing GTA: San Andreas data...");
        executor.execute(() -> {
            try {
                File dir = getGameStorageDir(this);
                for (int i = 0; i < obbs.size(); i++) {
                    File obb = obbs.get(i);
                    String label = obb.getName() + " (" + (i + 1) + "/" + obbs.size() + ")";
                    try (InputStream in = new FileInputStream(obb)) {
                        GameDataManager.importArchive(in, obb.length(), dir, progressListener(label));
                    }
                }
                finishImport(dir);
            } catch (Exception e) {
                runOnUiThread(() -> {
                    showNeedGameData(0);
                    textStatus.setText("Could not read the installed game files (" + e.getMessage() + ").\n\n"
                            + "Copy the OBB from Android/obb/" + GameDataManager.GTASA_PACKAGE
                            + "/ to your Download folder and select it instead.");
                });
            }
        });
    }

    private void importPickedFiles(List<Uri> uris) {
        final List<Uri> ordered = new ArrayList<>(uris);
        ordered.sort((a, b) -> Integer.compare(
                GameDataManager.obbOrder(displayName(a)), GameDataManager.obbOrder(displayName(b))));

        setBusy("IMPORTING GAME DATA", "Importing selected file(s)...");
        executor.execute(() -> {
            try {
                File dir = getGameStorageDir(this);
                for (int i = 0; i < ordered.size(); i++) {
                    Uri uri = ordered.get(i);
                    String label = displayName(uri) + " (" + (i + 1) + "/" + ordered.size() + ")";
                    try (InputStream in = getContentResolver().openInputStream(uri)) {
                        if (in == null) throw new Exception("cannot open " + displayName(uri));
                        GameDataManager.importArchive(in, sizeOf(uri), dir, progressListener(label));
                    }
                }
                finishImport(dir);
            } catch (Exception e) {
                runOnUiThread(() -> {
                    showNeedGameData(0);
                    textStatus.setText("Import failed: " + e.getMessage()
                            + "\n\nSelect the GTA: San Andreas OBB file (it is a zip archive).");
                });
            }
        });
    }

    private void finishImport(File dir) {
        GameDataManager.Result result = GameDataManager.validate(dir);
        runOnUiThread(() -> {
            if (result.isValid()) checkState();
            else showValidationFailure(result);
        });
    }

    private GameDataManager.ProgressListener progressListener(String label) {
        return (read, total, entry) -> runOnUiThread(() -> {
            int pct = total > 0 ? (int) Math.min(100, read * 100 / total) : 0;
            progressBar.setProgress(pct);
            textStatus.setText(String.format("Importing %s: %d%%\n%s", label, pct, entry));
        });
    }

    private String displayName(Uri uri) {
        try (Cursor c = getContentResolver().query(uri, new String[] {OpenableColumns.DISPLAY_NAME}, null, null, null)) {
            if (c != null && c.moveToFirst() && !c.isNull(0)) return c.getString(0);
        } catch (Exception ignored) {}
        String last = uri.getLastPathSegment();
        return last != null ? last : "file";
    }

    private long sizeOf(Uri uri) {
        try (Cursor c = getContentResolver().query(uri, new String[] {OpenableColumns.SIZE}, null, null, null)) {
            if (c != null && c.moveToFirst() && !c.isNull(0)) return c.getLong(0);
        } catch (Exception ignored) {}
        return -1;
    }

    // ------------------------------------------------------------------
    // Kingmakers RP content pack (our own files only)
    // ------------------------------------------------------------------

    private void downloadContentPack() {
        setBusy("KINGMAKERS RP CONTENT", "Querying Kingmakers RP updater...");
        progressBar.setProgress(0);

        executor.execute(() -> {
            try {
                String apiUrl = getString(R.string.backend_api_url) + "/api/launcher/config";
                Response cfgRes = httpClient.newCall(new Request.Builder().url(apiUrl).build()).execute();
                if (!cfgRes.isSuccessful() || cfgRes.body() == null) {
                    throw new Exception("Unable to reach updater API (" + cfgRes.code() + ")");
                }

                JSONObject cfg = new JSONObject(cfgRes.body().string());
                String packUrl = cfg.optString("contentPackUrl", "");
                String expectedSha256 = cfg.optString("contentPackSha256", "");
                String version = cfg.optString("latestVersion", "1.0.0");
                if (packUrl.isEmpty()) throw new Exception("Content pack URL not provided by server");

                File packZip = new File(getExternalCacheDir(), "km_content_pack.zip");
                if (packZip.exists()) packZip.delete();

                Response dlRes = httpClient.newCall(new Request.Builder().url(packUrl).build()).execute();
                if (!dlRes.isSuccessful() || dlRes.body() == null) {
                    throw new Exception("Download failed with HTTP " + dlRes.code());
                }

                ResponseBody body = dlRes.body();
                long contentLength = body.contentLength();
                MessageDigest sha = MessageDigest.getInstance("SHA-256");
                try (InputStream in = body.byteStream(); FileOutputStream out = new FileOutputStream(packZip)) {
                    byte[] buffer = new byte[65536];
                    long totalRead = 0;
                    long lastUi = 0;
                    int n;
                    while ((n = in.read(buffer)) != -1) {
                        out.write(buffer, 0, n);
                        sha.update(buffer, 0, n);
                        totalRead += n;
                        long now = System.currentTimeMillis();
                        if (now - lastUi > 300) {
                            lastUi = now;
                            final long cur = totalRead;
                            runOnUiThread(() -> {
                                int pct = contentLength > 0 ? (int) Math.min(99, cur * 100 / contentLength) : 0;
                                progressBar.setProgress(pct);
                                textStatus.setText(String.format("Downloading: %.1f MB", cur / (1024.0 * 1024.0)));
                            });
                        }
                    }
                }

                StringBuilder hex = new StringBuilder();
                for (byte b : sha.digest()) hex.append(String.format("%02x", b));
                if (!expectedSha256.isEmpty() && !expectedSha256.equalsIgnoreCase(hex.toString())) {
                    packZip.delete();
                    throw new Exception("Checksum mismatch, please retry");
                }

                runOnUiThread(() -> textStatus.setText("Installing Kingmakers RP content..."));
                File dir = getGameStorageDir(this);
                unzipOwnContent(packZip, dir);
                packZip.delete();

                File marker = new File(dir, CONTENT_MARKER);
                if (marker.getParentFile() != null) marker.getParentFile().mkdirs();
                try (FileWriter fw = new FileWriter(marker, false)) { fw.write(version); }

                runOnUiThread(this::checkState);
            } catch (Exception e) {
                runOnUiThread(() -> {
                    showNeedContent();
                    btnAction.setText("RETRY DOWNLOAD");
                    textStatus.setText("Error: " + e.getMessage());
                });
            }
        });
    }

    /** Extracts only our own content folders, so it can never overwrite the player's game data. */
    private static void unzipOwnContent(File zipFile, File targetDir) throws Exception {
        String canonicalTarget = targetDir.getCanonicalPath() + File.separator;
        byte[] buffer = new byte[32768];
        try (ZipInputStream zis = new ZipInputStream(new BufferedInputStream(new FileInputStream(zipFile)))) {
            ZipEntry entry;
            while ((entry = zis.getNextEntry()) != null) {
                String name = entry.getName().replace('\\', '/');
                File outFile = new File(targetDir, name);
                if (!GameDataManager.isOwnContent(name)
                        || !outFile.getCanonicalPath().startsWith(canonicalTarget)) {
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
                        while ((len = zis.read(buffer)) > 0) fos.write(buffer, 0, len);
                    }
                }
                zis.closeEntry();
            }
        }
    }

    // ------------------------------------------------------------------
    // Per-launch setup (called from LauncherActivity and Samp.kt)
    // ------------------------------------------------------------------

    public static void ensureGameFiles(Context ctx, String nick, String ip, int port) {
        File baseDir = getGameStorageDir(ctx);
        try {
            if (!baseDir.exists()) baseDir.mkdirs();
            GameDataManager.removePlaceholderFiles(baseDir);
            extractBundledContent(ctx, baseDir);

            File sampDir = new File(baseDir, "SAMP");
            sampDir.mkdirs();
            File kmDir = new File(baseDir, "kingmakers");
            kmDir.mkdirs();

            String iniContent = "[client]\n" +
                    "name=" + nick + "\n" +
                    "ip=" + ip + "\n" +
                    "host=" + ip + "\n" +
                    "port=" + port + "\n" +
                    "debug=0\n\n" +
                    "[gui]\n" +
                    "Font=visby-round-cf-extra-bold.ttf\n" +
                    "fps=60\n" +
                    "FontSize=30.0\n" +
                    "ChatFontSize=-1\n" +
                    "FontOutline=2\n";
            try (FileWriter fw = new FileWriter(new File(sampDir, "settings.ini"), false)) {
                fw.write(iniContent);
            }

            String kmContent = "[Connection]\n" +
                    "host = " + ip + "\n" +
                    "port = " + port + "\n" +
                    "server_name = Kingmakers RP | Mobile Roleplay | v1.0.0\n" +
                    "version = 1.0.0\n" +
                    "[Client]\n" +
                    "name = " + nick + "\n" +
                    "fast_load = true\n" +
                    "debug_logs = false\n";
            try (FileWriter fw = new FileWriter(new File(kmDir, "server.ini"), false)) {
                fw.write(kmContent);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /** Copies our bundled APK assets (assets/game/<own content dirs>) into the storage dir. */
    private static void extractBundledContent(Context ctx, File targetDir) {
        extractAssetFolder(ctx, "game", "", targetDir);
    }

    private static void extractAssetFolder(Context ctx, String assetPath, String relPath, File targetDir) {
        try {
            String[] list = ctx.getAssets().list(assetPath);
            if (list == null) return;
            for (String file : list) {
                String subAsset = assetPath + "/" + file;
                String subRel = relPath + file;
                String[] subList = ctx.getAssets().list(subAsset);
                boolean isDir = subList != null && subList.length > 0;

                if (isDir) {
                    // Descend only into directories that are, or lead to, our own content
                    String dirRel = subRel + "/";
                    if (!GameDataManager.isOwnContent(dirRel) && !leadsToOwnContent(dirRel)) continue;
                    File subTarget = new File(targetDir, file);
                    subTarget.mkdirs();
                    extractAssetFolder(ctx, subAsset, dirRel, subTarget);
                } else {
                    if (!GameDataManager.isOwnContent(subRel)) continue;
                    File outFile = new File(targetDir, file);
                    if (outFile.exists() && outFile.length() > 0) continue;
                    try (InputStream in = ctx.getAssets().open(subAsset);
                         OutputStream out = new FileOutputStream(outFile)) {
                        byte[] buf = new byte[32768];
                        int len;
                        while ((len = in.read(buf)) > 0) out.write(buf, 0, len);
                    }
                }
            }
        } catch (Exception ignored) {}
    }

    private static boolean leadsToOwnContent(String dirRel) {
        for (String prefix : GameDataManager.OWN_CONTENT_DIRS) {
            if (prefix.regionMatches(true, 0, dirRel, 0, dirRel.length())) return true;
        }
        return false;
    }

    // ------------------------------------------------------------------
    // Launch
    // ------------------------------------------------------------------

    private void launchGame() {
        GameDataManager.Result result = GameDataManager.validate(getGameStorageDir(this));
        if (!result.isValid()) {
            showValidationFailure(result);
            return;
        }
        try {
            ensureGameFiles(this, playerNick, serverIp, serverPort);

            Intent gameIntent = new Intent(this, com.russia.game.core.Samp.class);
            gameIntent.putExtra("player_name", playerNick);
            gameIntent.putExtra("name", playerNick);
            gameIntent.putExtra("nickname", playerNick);
            gameIntent.putExtra("server_ip", serverIp);
            gameIntent.putExtra("ip", serverIp);
            gameIntent.putExtra("host", serverIp);
            gameIntent.putExtra("server_port", serverPort);
            gameIntent.putExtra("port", serverPort);
            startActivity(gameIntent);
            finish();
        } catch (Exception e) {
            Toast.makeText(this, "Failed to launch game engine: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }
}
