/*
 * Copyright (C) 2018 The Android Open Source Project
 * Copyright (C) 2021 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
