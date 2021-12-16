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
import android.content.Context;

public class DispatchHelper {
    static {
       System.loadLibrary("VsockMsgDispatch");
    }       
    private static final String TAG = "DispatchHelper";
    private static DispatchHelper single_instance = null;
    public Context mContext;
    private DispatchHelper() {
    }
    public static DispatchHelper getInstance() {
       if (single_instance == null) {
          single_instance = new DispatchHelper();
       }
       return single_instance;
    }

    public native void registerComponent(String className);
    public native void sendMsg(String className, String msg, long handle);
    public native void start();
    public native void stop();

}
