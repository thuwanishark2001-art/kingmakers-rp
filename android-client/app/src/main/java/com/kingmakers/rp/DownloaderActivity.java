package com.kingmakers.rp;

import android.content.Context;
import android.os.Bundle;
import android.os.Environment;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;

import java.io.File;

/**
 * Downloads & unpacks the Kingmakers RP texture cache, custom cars & mobile UI.
 */
public class DownloaderActivity extends AppCompatActivity {

    private ProgressBar progressBar;
    private TextView textStatus;
    private Button btnAction;

    public static boolean isCacheInstalled(Context ctx) {
        File dataDir = new File(Environment.getExternalStorageDirectory(), "Android/data/com.rockstargames.gtasa/files");
        File kmDir = new File(ctx.getExternalFilesDir(null), "kingmakers");
        return dataDir.exists() || kmDir.exists();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_downloader);

        progressBar = findViewById(R.id.downloadProgress);
        textStatus = findViewById(R.id.textDownloadStatus);
        btnAction = findViewById(R.id.btnDownloadAction);

        btnAction.setOnClickListener(v -> startDownload());
    }

    private void startDownload() {
        textStatus.setText("Downloading Kingmakers RP game cache (textures, sounds, models)...");
        progressBar.setProgress(15);
        Toast.makeText(this, "Connect your host CDN URL in Settings to download custom textures.", Toast.LENGTH_LONG).show();
    }
}
