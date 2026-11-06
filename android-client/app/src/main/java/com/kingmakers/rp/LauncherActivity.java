package com.kingmakers.rp;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import android.content.ClipData;
import android.content.ClipboardManager;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import org.json.JSONObject;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;

/**
 * Kingmakers RP Mobile Launcher.
 * Allows player to choose roleplay nickname (Firstname_Lastname),
 * view real-time server status & player count from the backend,
 * configure server IP/port, and launch the game client.
 */
public class LauncherActivity extends AppCompatActivity {

    private static final String PREFS = "KingmakersRP";
    private static final String KEY_NICK = "player_nickname";
    private static final String KEY_SERVER_IP = "server_ip";
    private static final String KEY_SERVER_PORT = "server_port";

    private EditText editNickname;
    private TextView textServerStatus;
    private TextView textOnlineCount;
    private Button btnPlay;
    private Button btnDiscord;
    private Button btnSettings;

    private final OkHttpClient httpClient = new OkHttpClient();
    private SharedPreferences prefs;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_launcher);

        prefs = getSharedPreferences(PREFS, Context.MODE_PRIVATE);

        editNickname = findViewById(R.id.editNickname);
        textServerStatus = findViewById(R.id.textServerStatus);
        textOnlineCount = findViewById(R.id.textOnlineCount);
        btnPlay = findViewById(R.id.btnPlay);
        btnDiscord = findViewById(R.id.btnDiscord);
        btnSettings = findViewById(R.id.btnSettings);

        String savedNick = prefs.getString(KEY_NICK, "Marcus_King");
        editNickname.setText(savedNick);

        btnPlay.setOnClickListener(v -> onPlayClicked());
        btnDiscord.setOnClickListener(v -> {
            Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(getString(R.string.discord_url)));
            startActivity(intent);
        });
        btnSettings.setOnClickListener(v -> {
            startActivity(new Intent(this, SettingsActivity.class));
        });

        fetchServerStatus();
    }

    private void onPlayClicked() {
        String nick = editNickname.getText().toString().trim();
        if (!isValidRPName(nick)) {
            Toast.makeText(this, "Use RP format: Firstname_Lastname (e.g. Marcus_King)", Toast.LENGTH_LONG).show();
            return;
        }
        prefs.edit().putString(KEY_NICK, nick).apply();

        String ip = prefs.getString(KEY_SERVER_IP, getString(R.string.server_default_ip));
        int port = Integer.parseInt(prefs.getString(KEY_SERVER_PORT, getString(R.string.server_default_port)));

        // Check if game cache is installed; if not, go to downloader
        if (!DownloaderActivity.isCacheInstalled(this)) {
            Intent dIntent = new Intent(this, DownloaderActivity.class);
            dIntent.putExtra("nick", nick);
            dIntent.putExtra("ip", ip);
            dIntent.putExtra("port", port);
            startActivity(dIntent);
            return;
        }

        launchGameClient(nick, ip, port);
    }

    private boolean isValidRPName(String name) {
        if (name == null || name.length() < 7 || name.length() > 22) return false;
        String[] parts = name.split("_");
        if (parts.length != 2) return false;
        if (parts[0].length() < 3 || parts[1].length() < 3) return false;
        return Character.isUpperCase(parts[0].charAt(0)) && Character.isUpperCase(parts[1].charAt(0));
    }

    private void writeSampSettings(String nick, String ip, int port) {
        try {
            File baseDir = DownloaderActivity.getGameStorageDir(this);
            File sampDir = new File(baseDir, "SAMP");
            if (!sampDir.exists()) sampDir.mkdirs();
            File iniFile = new File(sampDir, "settings.ini");
            String iniContent = "[client]\n" +
                    "name=" + nick + "\n" +
                    "host=" + ip + "\n" +
                    "port=" + port + "\n" +
                    "debug=0\n";
            try (FileWriter fw = new FileWriter(iniFile, false)) {
                fw.write(iniContent);
            }

            File kmDir = new File(baseDir, "kingmakers");
            if (!kmDir.exists()) kmDir.mkdirs();
            File kmIni = new File(kmDir, "server.ini");
            String kmContent = "[Connection]\n" +
                    "host = " + ip + "\n" +
                    "port = " + port + "\n" +
                    "server_name = Kingmakers RP | Mobile Roleplay | v1.0.0\n" +
                    "version = 1.0.0\n" +
                    "[Client]\n" +
                    "name = " + nick + "\n" +
                    "fast_load = true\n" +
                    "skip_intro_movies = true\n" +
                    "high_fps_mode = true\n" +
                    "fps_cap = 60\n";
            try (FileWriter fw = new FileWriter(kmIni, false)) {
                fw.write(kmContent);
            }
        } catch (Exception ignored) {}
    }

    private void launchGameClient(String nick, String ip, int port) {
        writeSampSettings(nick, ip, port);

        String[] candidatePackages = new String[] {
            "com.rockstargames.gtasa",
            "com.kingmakers.game",
            "com.samp.mobile",
            "com.samp.x1y2z",
            "ru.unisamp_mobile.game",
            "com.liverussia.cr",
            "com.arizona.game",
            "com.blackrussia.online",
            "com.mordor.game"
        };

        Intent launchIntent = null;
        for (String pkg : candidatePackages) {
            Intent it = getPackageManager().getLaunchIntentForPackage(pkg);
            if (it != null) {
                launchIntent = it;
                break;
            }
        }

        if (launchIntent != null) {
            launchIntent.putExtra("player_name", nick);
            launchIntent.putExtra("name", nick);
            launchIntent.putExtra("nickname", nick);
            launchIntent.putExtra("server_ip", ip);
            launchIntent.putExtra("ip", ip);
            launchIntent.putExtra("host", ip);
            launchIntent.putExtra("server_port", port);
            launchIntent.putExtra("port", port);
            startActivity(launchIntent);
        } else {
            showGameCoreMissingDialog(nick, ip, port);
        }
    }

    private void showGameCoreMissingDialog(String nick, String ip, int port) {
        String msg = "The Kingmakers RP Launcher has prepared your game files and configured your player nickname (" + nick + ").\n\n" +
                "To start the 3D roleplay game, your device needs the GTA:SA / SA-MP Mobile Game Client installed.\n\n" +
                "Server Connection Details:\n" +
                "IP: " + ip + "\n" +
                "Port: " + port;

        new AlertDialog.Builder(this)
                .setTitle("GTA:SA / SA-MP Client Required")
                .setMessage(msg)
                .setPositiveButton("Get SA-MP Client", (dialog, which) -> {
                    try {
                        Intent playIntent = new Intent(Intent.ACTION_VIEW,
                                Uri.parse("https://play.google.com/store/apps/details?id=com.samp.x1y2z"));
                        startActivity(playIntent);
                    } catch (Exception e) {
                        Intent webIntent = new Intent(Intent.ACTION_VIEW,
                                Uri.parse("https://sa-mp-launcher.en.uptodown.com/android"));
                        startActivity(webIntent);
                    }
                })
                .setNeutralButton("Copy IP:Port", (dialog, which) -> {
                    ClipboardManager cm = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
                    if (cm != null) {
                        ClipData clip = ClipData.newPlainText("Kingmakers RP Server", ip + ":" + port);
                        cm.setPrimaryClip(clip);
                        Toast.makeText(this, "Copied " + ip + ":" + port + " to clipboard!", Toast.LENGTH_SHORT).show();
                    }
                })
                .setNegativeButton("Close", null)
                .show();
    }

    private void fetchServerStatus() {
        String url = getString(R.string.backend_api_url) + "/api/status";
        Request req = new Request.Builder().url(url).build();
        httpClient.newCall(req).enqueue(new Callback() {
            @Override
            public void onFailure(Call call, IOException e) {
                runOnUiThread(() -> {
                    textServerStatus.setText("Offline");
                    textServerStatus.setTextColor(getColor(R.color.km_red));
                    textOnlineCount.setText("0 / 100");
                });
            }

            @Override
            public void onResponse(Call call, Response response) throws IOException {
                if (!response.isSuccessful()) return;
                try {
                    String json = response.body().string();
                    JSONObject obj = new JSONObject(json);
                    boolean online = obj.optBoolean("online", false);
                    int players = obj.optInt("players", 0);
                    int max = obj.optInt("maxPlayers", 100);
                    runOnUiThread(() -> {
                        if (online) {
                            textServerStatus.setText("Online");
                            textServerStatus.setTextColor(getColor(R.color.km_green));
                            textOnlineCount.setText(players + " / " + max);
                        } else {
                            textServerStatus.setText("Maintenance");
                            textServerStatus.setTextColor(getColor(R.color.km_red));
                        }
                    });
                } catch (Exception ignored) {}
            }
        });
    }
}
