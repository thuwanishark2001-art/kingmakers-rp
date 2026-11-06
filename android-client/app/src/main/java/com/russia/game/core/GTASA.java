package com.russia.game.core;

import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.KeyEvent;

import com.bytedance.shadowhook.ShadowHook;
import com.russia.launcher.utils.MainUtils;
import com.wardrumstudios.utils.WarMedia;

public class GTASA extends WarMedia {
   // public static GTASA gtasaSelf = null;
    static String vmVersion;
    private boolean once = false;

    static {
        try {
            ShadowHook.init(new ShadowHook.ConfigBuilder()
                    .setMode(ShadowHook.Mode.UNIQUE)
                    .build());
        } catch (Throwable t) {
            System.err.println("ShadowHook.init failed: " + t.getMessage());
        }

        vmVersion = null;
        System.out.println("**** Loading SO's");
        try {
            System.loadLibrary("SCAnd");
        } catch (Throwable t) {
            System.out.println("libSCAnd load note: " + t.getMessage());
        }
        try {
            System.loadLibrary("OpenAL64");
        } catch (Throwable t) {
            System.out.println("libOpenAL64 load note: " + t.getMessage());
        }
        try {
            System.loadLibrary("bass");
        } catch (Throwable t) {
            System.out.println("libbass load note: " + t.getMessage());
        }
        try {
            System.loadLibrary("bass_ssl");
        } catch (Throwable t) {
            System.out.println("libbass_ssl load note: " + t.getMessage());
        }
        try {
            System.loadLibrary("GTASA");
        } catch (Throwable t) {
            System.err.println("CRITICAL: Failed to load libGTASA: " + t.getMessage());
        }
        try {
            System.loadLibrary("multiplayer");
        } catch (Throwable t) {
            System.err.println("CRITICAL: Failed to load libmultiplayer: " + t.getMessage());
        }
    }

    public static void staticEnterSocialClub()
    {
      //  gtasaSelf.EnterSocialClub();
    }

    public static void staticExitSocialClub() {
        //gtasaSelf.ExitSocialClub();
    }

    public void AfterDownloadFunction() {

    }

    public void EnterSocialClub() {

    }

    public void ExitSocialClub() {

    }

    public boolean ServiceAppCommand(String str, String str2)
    {
        return false;
    }

    public int ServiceAppCommandValue(String str, String str2)
    {
        return 0;
    }

    public native void main();

    public void onActivityResult(int i, int i2, Intent intent)
    {
        super.onActivityResult(i, i2, intent);
    }

    public void onConfigurationChanged(Configuration configuration)
    {
        super.onConfigurationChanged(configuration);
    }

    public void onCreate(Bundle bundle)
    {
        if(!once)
        {
            once = true;
        }

        System.out.println("GTASA onCreate");
      //  gtasaSelf = this;
      //  wantsAccelerometer = true;

        super.onCreate(bundle);
    }

    public void onDestroy()
    {
        System.out.println("GTASA onDestroy");
        super.onDestroy();
    }

    public boolean onKeyDown(int i, KeyEvent keyEvent)
    {
        return super.onKeyDown(i, keyEvent);
    }

    public void onPause()
    {
        System.out.println("GTASA onPause");
        super.onPause();
    }

    public void onRestart()
    {
        System.out.println("GTASA onRestart");
        super.onRestart();
    }

    public void onResume()
    {
        System.out.println("GTASA onResume");
        super.onResume();
    }

    public void onStart()
    {
        System.out.println("GTASA onStart");
        super.onStart();
    }

    public void onStop()
    {
        System.out.println("GTASA onStop");
        super.onStop();
    }

    public native void setCurrentScreenSize(int i, int i2);
}