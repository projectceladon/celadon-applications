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
import java.util.HashMap;
import java.util.List;
import com.intel.clipboardagent.DispatchHelper;
import android.content.Context;
import android.app.ActivityManager;
import android.app.ActivityManager.RecentTaskInfo;
import android.content.pm.PackageManager;

import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND;
import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_VISIBLE;
import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_CACHED;
import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_GONE;

public class AppstatusComponent {
    private static final String TAG = "AppstatusComponent";
    private static AppstatusComponent single_instance = null;
    private DispatchHelper dH;
    private ActivityManager mActivityManager;
    private HashMap<Integer, Integer> uidPrevImpMap = new HashMap<Integer, Integer>();
    private static final int FOREGROUND_IMPORTANCE_CUTOFF = ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND_SERVICE; 

    private AppstatusComponent(){
    } 

    public static AppstatusComponent getInstance() {
       if (single_instance == null) {
          single_instance = new AppstatusComponent();
       }
       return single_instance;
    }

    public void init() {
	dH = DispatchHelper.getInstance();
        Log.d(TAG, "addOnUidImportanceListener");
        mActivityManager = (ActivityManager) dH.mContext.getSystemService(Context.ACTIVITY_SERVICE);
        mActivityManager.addOnUidImportanceListener(mOnUidImportanceListener, FOREGROUND_IMPORTANCE_CUTOFF);
       	    
    }

    public void stop() {
        if (mActivityManager != null) {
            Log.d(TAG, "removeOnUidImportanceListener");
            mActivityManager.removeOnUidImportanceListener(mOnUidImportanceListener);
        }
    }

    private String getPackageName(int uid) {
	String packageName = "";
        String[] packages = dH.mContext.getPackageManager().getPackagesForUid(uid);
        if (packages == null) {
            Log.d(TAG, "No package is associated with that uid, do nothing");
        } else if (packages.length == 1) {
	    packageName = packages[0];
	} else {
	    Log.d(TAG, "Multiple packages associated with the uid, should see what to do");
	}
	return packageName;
    }

    private boolean isHomeForeground(){
	try {
            int homeId = dH.mContext.getPackageManager().getPackageUid("com.android.launcher3", 0);
            if (mActivityManager.getUidImportance(homeId) == IMPORTANCE_FOREGROUND)
	       return true;
	} catch (PackageManager.NameNotFoundException e) {
	    e.printStackTrace();
	}
        return false;
    }

    private boolean killLG(String appName) {
        List<ActivityManager.RecentTaskInfo> recentTasks = mActivityManager.getRecentTasks(10, 0);
	boolean kill = true;
        for (ActivityManager.RecentTaskInfo taskInfo : recentTasks) {
            Log.d(TAG, "In AHSFunc, taskinfo.isRunning " + taskInfo.isRunning);
            Log.d(TAG, "In AHSFunc, taskinfo.isVisible " + taskInfo.isVisible());
            if ((taskInfo.baseActivity != null) && taskInfo.isRunning && taskInfo.isVisible() && appName.equals(taskInfo.baseActivity.getPackageName())) {
		kill = false;
	        break;
	    }
	}
	return kill;
    }

    private void dumpTaskInfo() {
        List<ActivityManager.RecentTaskInfo> recentTasks = mActivityManager.getRecentTasks(10, 0);
        for (ActivityManager.RecentTaskInfo taskInfo : recentTasks) {
		Log.d(TAG, "taskInfo = " + taskInfo.toString());
	}
    }

    private final ActivityManager.OnUidImportanceListener mOnUidImportanceListener =
        new ActivityManager.OnUidImportanceListener() {
            @Override
             public void onUidImportance(final int uid, final int importance){
		 Log.d(TAG, "In onUidImportance event, uid = " + uid + " and importance = " + importance);
		 String appName = getPackageName(uid);
		 if (appName.isEmpty()) {
                     Log.d(TAG, "No app associated with uid, so return");
                     return;
		 }
		 Log.d(TAG, "In onUidImportance Listener, processing App = " + appName);
		 dumpTaskInfo();
		 if (uidPrevImpMap.containsKey(uid)) {
                     int prevImp = uidPrevImpMap.get(uid);
		     Log.d(TAG, "prev imp value of uid " + uid + " is " + prevImp);
		     if (prevImp == IMPORTANCE_FOREGROUND) {
		         if (importance == IMPORTANCE_GONE) {
	                     Log.d(TAG, "App with uid " + uid + " killed, send message to host");
                             Log.d(TAG, "1:: Sending message to host");
                             dH.sendMsg("AppstatusComponent", appName, 0);
			 } else if(importance >= IMPORTANCE_VISIBLE) { // && importance <= IMPORTANCE_CACHED) {
	                     Log.d(TAG, "App with uid " + uid + " moved from foreground to background");
			     if (killLG(appName)) {
                                 Log.d(TAG, "2:: Sending message to host");
                                 dH.sendMsg("AppstatusComponent", appName, 0);
			     }
			 }
		     }
                     if (importance == IMPORTANCE_GONE) {
	                 Log.d(TAG, "App with uid " + uid + " killed, remove from the map");
                         uidPrevImpMap.remove(uid);
		     } else {
                         uidPrevImpMap.put(uid, importance);
		     }
                 } else {
                     uidPrevImpMap.put(uid, importance);
		 }
             }
        };

    /*public void processMsg(String content, long handle) {
	try {
            int uid = dH.mContext.getPackageManager().getPackageUid(content, 0);
	    // can diff uid have same handle?
            uidAppnameMap.put(uid, content);
            uidChannelMap.put(uid, handle);
	} catch (PackageManager.NameNotFoundException e) {
	    e.printStackTrace();    	
	}
    }*/
}
