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
import android.util.Log;
import com.intel.clipboardagent.DispatchHelper;
import android.content.Context;
import android.app.ActivityManager;
import android.content.pm.PackageManager;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;
import android.content.ComponentName;

public class NotificationListener extends NotificationListenerService {
    private static final String TAG = "NotificationListener";
    private static NotificationsChangedListener sNotificationsChangedListener;
    
    public interface NotificationsChangedListener {
        void onNotificationPosted(StatusBarNotification sbn);
    }

    public NotificationListener() {
	sNotificationsChangedListener = null;
    }

    public static void setNotificationsChangedListener(NotificationsChangedListener listener) {
        sNotificationsChangedListener = listener;
    }
    
    public static void removeNotificationsChangedListener() {
        sNotificationsChangedListener = null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void onNotificationPosted(StatusBarNotification sbn) {
        Log.d(TAG, "In onNotificationPosted");
	if (sNotificationsChangedListener != null) {
	    sNotificationsChangedListener.onNotificationPosted(sbn);
	}
    }
}

