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
import com.intel.clipboardagent.DispatchHelper;
import android.content.Context;
import android.app.ActivityManager;
import android.content.pm.PackageManager;

import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND;
//import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND_SERVICE;
import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_VISIBLE;
import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_CACHED;
import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_GONE;

public class AppstatusComponent {
    private static final String TAG = "AppstatusComponent";
    private static AppstatusComponent single_instance = null;
    private DispatchHelper dH;
    private ActivityManager mActivityManager;
    //private HashMap<Integer, Long> uidChannelMap = new HashMap<Integer, Long>();
    //private HashMap<Integer, String> uidAppnameMap = new HashMap<Integer, String>();
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

    /*private static boolean isInForeground(int importance) {
        return ((importance == IMPORTANCE_FOREGROUND) || (importance == IMPORTANCE_FOREGROUND_SERVICE));
    }*/

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


    private final ActivityManager.OnUidImportanceListener mOnUidImportanceListener =
        new ActivityManager.OnUidImportanceListener() {
            @Override
             public void onUidImportance(final int uid, final int importance){
		 Log.d(TAG, "In onUidImportance event, uid = " + uid + " and importance = " + importance);
		 // Notify host when LG instance corresponding to app has to be killed
		 if (uidPrevImpMap.containsKey(uid)) {
                     int prevImp = uidPrevImpMap.get(uid);
		     Log.d(TAG, "prev imp value of uid " + uid + " is " + prevImp);
		     if (prevImp == IMPORTANCE_FOREGROUND) {
		         if (importance == IMPORTANCE_GONE) {
	                     //Log.d(TAG, "App with uid " + uid + " moved from foreground to background/killed, send message to host");
	                     Log.d(TAG, "App with uid " + uid + " killed, send message to host");
			     String appName = getPackageName(uid);
		             if(!appName.isEmpty()) {
				 Log.d(TAG, "1:: Sending message to host");
	                         dH.sendMsg("AppstatusComponent", appName, 0);
			     }
			 } else if(importance >= IMPORTANCE_VISIBLE && importance <= IMPORTANCE_CACHED) {

	                     Log.d(TAG, "App with uid " + uid + " moved from foreground to background");
			     if (isHomeForeground()) {
				 Log.d(TAG, "Current foreground is home screen, which means this app might have crashed. So kill LG window");
			         String appName = getPackageName(uid);
			         if (!appName.isEmpty()) {
				    Log.d(TAG, "2:: Sending message to host");
	                            dH.sendMsg("AppstatusComponent", appName, 0);
				 }
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
