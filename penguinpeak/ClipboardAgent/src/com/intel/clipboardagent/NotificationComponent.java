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
import android.app.NotificationManager;
import java.util.List;

public class NotificationComponent {
    private static final String TAG = "NotificationComponent";
    private static NotificationComponent single_instance = null;
    private DispatchHelper dH;
    private NotificationListener mListener;

    private NotificationComponent(){
    } 

    public static NotificationComponent getInstance() {
       if (single_instance == null) {
          single_instance = new NotificationComponent();
       }
       return single_instance;
    }

    public void init() {
	dH = DispatchHelper.getInstance();
        Log.d(TAG, "In init");
	mListener = new NotificationListener();
	mListener.setNotificationsChangedListener(mNotificationsChangedListener);
        ComponentName cn = ComponentName.unflattenFromString("com.intel.clipboardagent/com.intel.clipboardagent.NotificationListener");
        NotificationManager nm = dH.mContext.getSystemService(NotificationManager.class);
	if (nm.isNotificationListenerAccessGranted(cn)) {
		Log.d(TAG, "Has notification acess");
        }
    }

    public void stop() {
	Log.d(TAG, "In stop");    
        if (mListener != null) {
	    mListener.removeNotificationsChangedListener();
        }
    }

    private final NotificationListener.NotificationsChangedListener mNotificationsChangedListener = new NotificationListener.NotificationsChangedListener() {
        @Override
        public void onNotificationPosted(StatusBarNotification sbn) {
            Log.d(TAG, "In NotificationComponent onNotificationPosted");
	    Log.d(TAG, "ID :" + sbn.getId() + "\t" + sbn.getNotification().tickerText +"\t" + sbn.getPackageName());
	    Log.d(TAG, "ID :" + sbn.isGroup() + "\t" + sbn.getKey() +"\t" + sbn.toString() +"\t" + sbn.getNotification().priority);
	    NotificationData notificationdata = new NotificationData();
	    notificationdata.packageName = sbn.getPackageName();
	    notificationdata.key = sbn.getKey();
	    notificationdata.groupKey = sbn.getGroupKey();
	    if (sbn.getNotification().tickerText != null) {
                notificationdata.message = sbn.getNotification().tickerText.toString();
            }
	    notificationdata.priority = sbn.getNotification().priority;
	    dH.sendMsg("NotificationComponent", notificationdata, 0);
        }
    };	

}

