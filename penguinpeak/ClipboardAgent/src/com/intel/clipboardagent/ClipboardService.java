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
import android.os.IBinder;
import android.util.Log;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import java.io.IOException;
import java.nio.charset.StandardCharsets;

import com.intel.clipboardagent.VsockClientImpl;
import com.intel.clipboardagent.VsockAddress;

public class ClipboardService extends Service{
    private static final String TAG = "ClipboardAgent";
    private static final String CLIPBOARD_SERVICE_LABEL = "IntelClipboardService";
    private static final int DEFAULT_DATA_LENGTH = 4096;
    private static final int MAX_DATA_LENGTH = 512*1024;
    private ExecutorService mThreadPool = Executors.newSingleThreadExecutor();
    private ClipboardManager mClipboardManager;
    private Vsock mVsock;
    private VsockAddress mVsockAddress;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
	Log.d(TAG, "In ClipboardService onCreate");

        mClipboardManager =
                (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
        mClipboardManager.addPrimaryClipChangedListener(
                mOnPrimaryClipChangedListener);
        // TODO: remove hard code on vsock port
        mVsockAddress = new VsockAddress(VsockAddress.VMADDR_CID_HOST, 77777);
        mVsock = new Vsock(mVsockAddress);

        mThreadPool.execute(new HandleHostVsockContent());
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
		    // This clip originated from the same service, suppress it.
		    if (CLIPBOARD_SERVICE_LABEL.equals(mclipData.getDescription().getLabel())) {
                        return;
		    }
                    CharSequence mText = mclipData.getItemAt(0).getText();
                    byte[] mBytes = mText.toString().getBytes(StandardCharsets.UTF_8);

                    try{
                        mVsock.getOutputStream().writeInt(mBytes.length);
                        int writeLength = (mBytes.length < MAX_DATA_LENGTH) ? mBytes.length : MAX_DATA_LENGTH;
                        // If Clipboard is cleared, nothing to send
                        if (writeLength > 0) {
                           mVsock.getOutputStream().write(mBytes, 0, writeLength);
                        }
                    } catch (IOException exception) {
                        Log.e(TAG, "Error on handling clipboard data: " + exception.getMessage());
                    }
                }
            };

    // Class HandleHostVsockContent should receive vsock data from remote host
    private class HandleHostVsockContent implements Runnable {
        private static final String TAG = "ClipboardAgent";

        private HandleHostVsockContent() {
        }

        @Override
        public void run() {
            // TODO: Data length is hard code here for 4096.
            byte[] buffer = new byte[DEFAULT_DATA_LENGTH];
	    try {
	       mVsock.connect();
            } catch (IOException exception) {
	       Log.e(TAG, "Failed to connect: " + exception.getMessage());
	    }
            while (true) {
                boolean bReconnect = false;
                byte[] bytes = buffer;
                String content = "";
                try {
                    int length = mVsock.getInputStream().readInt();
                    if (length < 0 || length > MAX_DATA_LENGTH) {
                        Log.wtf(TAG, "Unexpected data size :"+length, new Exception("Unexpected data size"));
                        continue;
                    }

                    if (length > DEFAULT_DATA_LENGTH) {
                       bytes = new byte[length];
                    }

                    if (length > 0) {
                        mVsock.getInputStream().read(bytes, 0, length);
                        content = new String(bytes, 0, length, StandardCharsets.UTF_8);
                    }
                    ClipData mclipData = mClipboardManager.getPrimaryClip();
                    mclipData = ClipData.newPlainText(CLIPBOARD_SERVICE_LABEL, content);
                    mClipboardManager.setPrimaryClip(mclipData);

                } catch (IOException exception) {
                    if (exception.toString().contains("Connection reset") ||
                        exception.toString().contains("Connection is closed by peer")) {
                        Log.e(TAG, "Connection reset, attempting to reconnect");
			bReconnect = true;
                    } else {
                        Log.e(TAG, "Error on handling host Vsock: " + exception.getMessage());
                    }
                }
                if (bReconnect) {
                    try {
                        mVsock.close();
                    } catch (IOException e) {
                        Log.e(TAG, "Failed to close vsock: " + e.getMessage());
                    }
                    try {
                        mVsock = new Vsock(mVsockAddress);
                        mVsock.connect();
                        Thread.sleep(1000);
                    } catch (IOException e) {
                        Log.e(TAG, "Error reconnecting... " + e.getMessage());
                    } catch (InterruptedException x) {}
                }
            }
        }
    }
}
