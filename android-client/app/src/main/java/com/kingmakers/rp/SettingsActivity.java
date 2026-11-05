package com.kingmakers.rp;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;

public class SettingsActivity extends AppCompatActivity {

    private EditText editIp, editPort, editCdn;
    private Button btnSave;
    private SharedPreferences prefs;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);

        prefs = getSharedPreferences("KingmakersRP", Context.MODE_PRIVATE);

        editIp = findViewById(R.id.editServerIp);
        editPort = findViewById(R.id.editServerPort);
        editCdn = findViewById(R.id.editCdnUrl);
        btnSave = findViewById(R.id.btnSaveSettings);

        editIp.setText(prefs.getString("server_ip", getString(R.string.server_default_ip)));
        editPort.setText(prefs.getString("server_port", getString(R.string.server_default_port)));
        editCdn.setText(prefs.getString("cdn_url", "https://cdn.kingmakers-rp.com/cache.zip"));

        btnSave.setOnClickListener(v -> {
            prefs.edit()
                .putString("server_ip", editIp.getText().toString().trim())
                .putString("server_port", editPort.getText().toString().trim())
                .putString("cdn_url", editCdn.getText().toString().trim())
                .apply();
            Toast.makeText(this, "Settings saved!", Toast.LENGTH_SHORT).show();
            finish();
        });
    }
}
