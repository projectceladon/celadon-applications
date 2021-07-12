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

import android.app.Service;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import java.io.IOException;
import java.nio.charset.StandardCharsets;

import com.intel.penguinpeakutils.VsockClientImpl;
import com.intel.penguinpeakutils.VsockAddress;

public class ClipboardService extends Service{
    private static final String TAG = "PenguinPeakUtils";
    private ExecutorService mThreadPool = Executors.newSingleThreadExecutor();
    private ClipboardManager mClipboardManager;
    private Vsock mVsock;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();

        mClipboardManager =
                (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
        mClipboardManager.addPrimaryClipChangedListener(
                mOnPrimaryClipChangedListener);
        // TODO: remove hard code on vsock port
        mVsock = new Vsock(new VsockAddress(VsockAddress.VMADDR_CID_HOST, 77777));

        mThreadPool.execute(new HandleHostVsockContent(mVsock));
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        if (mClipboardManager != null) {
            Log.d(TAG, "removePrimaryClipChangedListener");
            mClipboardManager.removePrimaryClipChangedListener(
                    mOnPrimaryClipChangedListener);
        }
        try {
            mVsock.close();
        } catch (IOException exception) {
            Log.e(TAG, "Error on closing Vsock: " + exception.getMessage());
        }

    }

    private final ClipboardManager.OnPrimaryClipChangedListener mOnPrimaryClipChangedListener =
            new ClipboardManager.OnPrimaryClipChangedListener() {
                @Override
                public void onPrimaryClipChanged() {
                    ClipData mclipData = mClipboardManager.getPrimaryClip();
                    CharSequence mText = mclipData.getItemAt(0).getText();
                    byte[] mBytes = mText.toString().getBytes(StandardCharsets.UTF_8);

                    try{
                        mVsock.getOutputStream().write(mBytes, 0, mBytes.length);
                    } catch (IOException exception) {
                        Log.e(TAG, "Error on handling clipboard data: " + exception.getMessage());
                    }
                }
            };

    // Class HandleHostVsockContent should receive vsock data from remote host
    private class HandleHostVsockContent implements Runnable {
        private static final String TAG = "PenguinPeakUtils";
        private final Vsock mVsock;

        private HandleHostVsockContent(Vsock vsock) {
            mVsock = vsock;
        }

        @Override
        public void run() {
            // TODO: Data length is hard code here for 4096.
            byte[] bytes = new byte[4096];
            while (true) {
                try {
                    int length;

                    length = mVsock.getInputStream().read(bytes, 0, 4096);
		    if (length > 0) {
                        String content = new String(bytes, 0, length, StandardCharsets.UTF_8);

                        ClipData mclipData = mClipboardManager.getPrimaryClip();
                        mclipData = ClipData.newPlainText("PenguinPeak", content);
                        mClipboardManager.setPrimaryClip(mclipData);
		    }

                } catch (IOException exception) {
                    Log.e(TAG, "Error on handling host Vsock: " + exception.getMessage());
                }
            }
        }
    }
}
