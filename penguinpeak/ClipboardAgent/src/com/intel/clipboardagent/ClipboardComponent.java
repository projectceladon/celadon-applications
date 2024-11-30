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
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Intent;
import android.util.Log;
import com.intel.clipboardagent.DispatchHelper;
import android.content.Context;
import static android.content.Context.CLIPBOARD_SERVICE;

public class ClipboardComponent {
    private static final String TAG = "ClipboardComponent";
    private static final String CLIPBOARD_SERVICE_LABEL = "IntelClipboardService";
    private static ClipboardComponent single_instance = null;
    private ClipboardManager mClipboardManager;
    private DispatchHelper dH;
    private long mChannelHandle = 0;

    private ClipboardComponent(){
    } 

    public static ClipboardComponent getInstance() {
       if (single_instance == null) {
          single_instance = new ClipboardComponent();
       }
       return single_instance;
    }
  
    public void init() {
        dH = DispatchHelper.getInstance();
	Log.d(TAG, "addPrimaryClipChangedListener");
        mClipboardManager =
                (ClipboardManager) dH.mContext.getSystemService(CLIPBOARD_SERVICE);
        mClipboardManager.addPrimaryClipChangedListener(
                mOnPrimaryClipChangedListener);
    }

    public void stop() {
        if (mClipboardManager != null) {
            Log.d(TAG, "removePrimaryClipChangedListener");
            mClipboardManager.removePrimaryClipChangedListener(
                    mOnPrimaryClipChangedListener);
        }
    }
	    

    private final ClipboardManager.OnPrimaryClipChangedListener mOnPrimaryClipChangedListener =
        new ClipboardManager.OnPrimaryClipChangedListener() {
            @Override
            public void onPrimaryClipChanged() {
                ClipData mclipData = mClipboardManager.getPrimaryClip();
		// This clip originated from the same service, suppress it.
		if (CLIPBOARD_SERVICE_LABEL.equals(mclipData.getDescription().getLabel())) {
                    return;
		}
                CharSequence mText = mclipData.getItemAt(0).getText();
		dH.sendMsg("ClipboardComponent", mText.toString(), mChannelHandle);
            }
        };

    public void processMsg(String content, long handle) {
        ClipData mclipData = mClipboardManager.getPrimaryClip();
        mclipData = ClipData.newPlainText(CLIPBOARD_SERVICE_LABEL, content);
        mClipboardManager.setPrimaryClip(mclipData);
	mChannelHandle = handle;
    }

}
