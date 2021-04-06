package com.intel.penguinpeakutils;

import android.app.Application;
import android.content.Intent;
import android.util.Log;
import com.intel.penguinpeakutils.ClipboardService;

public class PenguinPeakUtils extends Application {
    private static final String TAG = "PenguinPeakUtils";
    private static final String SERVICE_NAME = "PenguinPeak";


    public void onCreate() {
        Log.d(TAG, "Application onCreate");
        super.onCreate();

        startService(new Intent(this, ClipboardService.class));
    }

    public void onTerminate() {
        super.onTerminate();
    }

}
