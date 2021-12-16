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

package com.intel.clipboardagent;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;
import com.intel.clipboardagent.DispatchHelper;
import android.content.Context;

public class GuestVsockCommService extends Service{
    private DispatchHelper dH;
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        dH = DispatchHelper.getInstance();
	dH.mContext = this.getApplicationContext(); 
	dH.registerComponent("ClipboardComponent");
	dH.registerComponent("AppstatusComponent");
	dH.start();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
	dH.stop();
    }
}
