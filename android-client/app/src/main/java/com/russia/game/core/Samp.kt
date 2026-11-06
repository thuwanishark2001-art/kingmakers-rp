package com.russia.game.core

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Intent
import android.content.pm.PackageManager
import android.media.AudioAttributes
import android.media.SoundPool
import android.net.Uri
import android.os.Bundle
import android.os.Vibrator
import android.util.Base64
import android.util.Log
import android.view.View
import android.view.ViewGroup
import android.view.ViewParent
import android.view.ViewStub
import android.view.WindowManager
import android.view.animation.AnimationUtils
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.constraintlayout.widget.ConstraintLayout
import com.russia.game.R
import com.russia.game.gui.hud.HudManager
import com.russia.launcher.async.task.CacheChecker.isGameCacheValid
import java.io.File
import java.io.IOException
import java.security.MessageDigest
import java.text.DecimalFormat
import java.text.DecimalFormatSymbols
import java.util.Locale
import java.util.Timer
import java.util.TimerTask

class Samp : GTASA() {
    private var mDialogClientSettings: DialogClientSettings? = null

    private external fun initSAMP(maxFps: Float, directory: String)

    override fun onCreate(bundle: Bundle?) {
        activity = this

        try {
            val wm = getSystemService(WINDOW_SERVICE) as? WindowManager
            val display = wm?.defaultDisplay
            maxFps = display?.refreshRate ?: 60f
        } catch (e: Exception) {
            maxFps = 60f
        }
        if (maxFps <= 0f) maxFps = 60f

        try {
            val internalDir = File(filesDir, "AudioConfig")
            clearDir(internalDir)
            copyFromAssets(internalDir)

            getExternalFilesDir(null)?.let { extFiles ->
                val extAudioDir = File(extFiles, "AudioConfig")
                copyFromAssets(extAudioDir)
            }
        } catch (e: Exception) {
            Log.e("Samp", "AudioConfig init note: ${e.message}")
        }

        val gameDir = getExternalFilesDir(null)?.absolutePath ?: filesDir.absolutePath
        val storagePath = if (gameDir.endsWith("/")) gameDir else "$gameDir/"

        try {
            val nick = intent?.getStringExtra("player_name") ?: intent?.getStringExtra("name") ?: "Marcus_King"
            val ip = intent?.getStringExtra("server_ip") ?: intent?.getStringExtra("ip") ?: "139.180.185.57"
            val port = intent?.getIntExtra("server_port", 7777) ?: 7777
            com.kingmakers.rp.DownloaderActivity.ensureGameFiles(this, nick, ip, port)
        } catch (e: Exception) {
            Log.e("Samp", "ensureGameFiles note: ${e.message}")
        }

        try {
            initSAMP(maxFps, storagePath)
        } catch (t: Throwable) {
            Log.e("Samp", "initSAMP error: ${t.message}")
        }

        super.onCreate(bundle)

        try {
            init()
        } catch (t: Throwable) {
            Log.e("Samp", "init GUI note: ${t.message}")
        }
    }
    private fun clearDir(dir: File) {
        try {
            if (dir.exists()) {
                dir.deleteRecursively()
                Log.d("AudioConfig", "Audio directory cleared: ${dir.absolutePath}")
            }
        } catch (e: IOException) {
            Log.e("AudioConfig", "Failed to clear audio dir: ${e.message}")
        }
    }

    private fun copyFromAssets(dir: File) {
        try {
            if (!dir.exists()) {
                dir.mkdirs()
            }

            val assetFiles = assets.list("AudioConfig") ?: return

            assetFiles.forEach { filename ->
                assets.open("AudioConfig/$filename").use { inputStream ->
                    File(dir, filename).outputStream().use { outputStream ->
                        inputStream.copyTo(outputStream)
                        Log.d("AudioConfig", "Copied audio: $filename → ${dir.absolutePath}")
                    }
                }
            }

        } catch (e: IOException) {
            Log.e("ShaderManager", "Failed to copy shaders: ${e.message}")
        }
    }

    fun init() {
        HudManager()

//        new Thread(() -> {
//            while (true) {
//        FrameLayout ui_layout = activity.findViewById(R.id.ui_layout);
//                int totalChildren = countAllChildren(ui_layout);
//                Log.d("df", "Total children count: " + totalChildren);
//
//                try {
//                    Thread.sleep(1000);
//                } catch (InterruptedException e) {
//                    throw new RuntimeException(e);
//                }
//            }
//        }).start();
    }

    fun countAllChildren(viewParent: ViewParent?): Int {
        if (viewParent == null || viewParent !is ViewGroup) {
            return 0
        }
        val childCount = viewParent.childCount
        var totalCount = childCount
        for (i in 0 until childCount) {
            val childView = viewParent.getChildAt(i)
            if (childView is ViewGroup) {
                totalCount += countAllChildren(childView)
            }
        }
        return totalCount
    }

    fun hideLoadingScreen() {
        val task: TimerTask = object : TimerTask() {
            override fun run() {
                activity.runOnUiThread {
                    val loadscreen_main_layout = activity.findViewById<ConstraintLayout>(R.id.loadscreen_main_layout)
                    val parentContainer = loadscreen_main_layout.parent as ViewGroup
                    parentContainer.removeView(loadscreen_main_layout)
                }
            }
        }
        val timer = Timer("Timer")
        timer.schedule(task, 900L)
    }

    fun exitGame() {
        finishAndRemoveTask()
        System.exit(0)
    }

    fun goVibrate(milliseconds: Int) {
        if (vibrator.hasVibrator()) {
            vibrator.vibrate(milliseconds.toLong())
        }
    }

    private fun openUrl(url: String) {
        runOnUiThread {
            val address = Uri.parse(url)
            val openlink = Intent(Intent.ACTION_VIEW, address)
            startActivity(openlink)
        }
    }

    private fun copyTextToBuffer(string: String) {
        runOnUiThread {
            val clipboardManager = this.getSystemService(CLIPBOARD_SERVICE) as ClipboardManager
            val clipData = ClipData.newPlainText("text", string)
            clipboardManager.setPrimaryClip(clipData)

            Toast.makeText(this, "Скопированно в буфер обмена ", Toast.LENGTH_SHORT).show()
        }
    }

    companion object {
        @JvmStatic
        lateinit var activity: AppCompatActivity

        @JvmStatic
        val vibrator by lazy { activity.getSystemService(VIBRATOR_SERVICE) as Vibrator }

        @JvmStatic
        var maxFps = 0f

        val windowManager by lazy { activity.getSystemService(WINDOW_SERVICE) as WindowManager }

        val formatter = DecimalFormatSymbols(Locale.getDefault()).run {
            groupingSeparator = '.'
            DecimalFormat("###,###.###", this)
        }

        const val INVALID_PLAYER_ID = 65535

        val soundPool = SoundPool.Builder()
            .setAudioAttributes(
                AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_GAME)
                    .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                    .build()
            )
            .build()

        @JvmStatic
        val clickAnim by lazy { AnimationUtils.loadAnimation(activity, R.anim.button_click) }

        // func
        external fun sendCommand(command: String?)
        external fun playUrlSound(url: String?)
        external fun gameFilesChecked(ok: Boolean)

        fun deInflatedViewStud(inflated: View?, id: Int, layout: Int) {
            activity.runOnUiThread {
                if (inflated == null) {
                    return@runOnUiThread
                }
                val parentContainer = inflated.parent as ViewGroup ?: return@runOnUiThread
                val index = parentContainer.indexOfChild(inflated)
                if (index == -1) {
                    return@runOnUiThread
                }
                parentContainer.removeViewAt(index)
                val viewStub = ViewStub(activity)
                viewStub.layoutResource = layout
                viewStub.id = id
                parentContainer.addView(viewStub, index)
            }
        }

        @JvmStatic
        private fun requestGameFilesCheck() {
            Thread {
                try {
                    activity.runOnUiThread { gameFilesChecked(true) }
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }.start()
        }

        @JvmStatic
        private fun getSignature(): String {
            var apkSignature: String? = null
            try {
                val packageInfo = activity.packageManager.getPackageInfo(
                    activity.packageName,
                    PackageManager.GET_SIGNATURES
                )
                for (signature in packageInfo.signatures!!) {
                    val md = MessageDigest.getInstance("SHA")
                    md.update(signature.toByteArray())
                    apkSignature = Base64.encodeToString(md.digest(), Base64.DEFAULT)
                }
            } catch (ignored: Exception) {
            }
            return apkSignature.toString()
        }

        val isTablet: Boolean
            get() = activity.resources.getBoolean(R.bool.is_tablet)
    }

    fun showClientSettings() {
        runOnUiThread {
            if (mDialogClientSettings != null) {
                mDialogClientSettings = null
            }
            mDialogClientSettings = DialogClientSettings()
            mDialogClientSettings?.show(supportFragmentManager, "test")
        }
    }
}
