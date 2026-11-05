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
import androidx.appcompat.app.AppCompatActivity;

import org.json.JSONObject;

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

    private void launchGameClient(String nick, String ip, int port) {
        // Direct intent to SA-MP / CRMP mobile client runtime
        Intent launchIntent = getPackageManager().getLaunchIntentForPackage("com.rockstargames.gtasa");
        if (launchIntent == null) {
            launchIntent = getPackageManager().getLaunchIntentForPackage("com.kingmakers.game");
        }
        if (launchIntent != null) {
            launchIntent.putExtra("player_name", nick);
            launchIntent.putExtra("server_ip", ip);
            launchIntent.putExtra("server_port", port);
            startActivity(launchIntent);
        } else {
            Toast.makeText(this, "Game client core not installed. Complete cache download first.", Toast.LENGTH_LONG).show();
        }
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
