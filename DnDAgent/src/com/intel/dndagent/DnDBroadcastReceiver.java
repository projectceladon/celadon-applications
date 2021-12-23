//
// Copyright (C) 2018 The Android Open Source Project
// Copyright (C) 2020 Intel Corporation

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//      http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.intel.dndagent;

import java.io.File;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Environment;
import android.util.Log;
import android.text.TextUtils;

import android.app.ActivityOptions;
import androidx.core.content.FileProvider;

public class DnDBroadcastReceiver extends BroadcastReceiver{
    private final static String TAG = "DnDBroadcastReceiver";
    @Override
    public void onReceive(Context context, Intent intent) {
        String data=intent.getExtras().get("data").toString();
        int displayId=intent.getIntExtra("displayId",-1);

	if (!data.contains("../") || !TextUtils.isEmpty(data)) {
            sendFiletoWX(context,data,displayId);
	} else {
            Log.e(TAG, "Unsupported filename format");
        }
    }

    private void sendFiletoWX(Context context,String data, int displayId) {
        String filePath = Environment.getExternalStorageDirectory() + "/Download/DnDData/" + data;

        File file = new File(filePath);

        Uri contentPath = FileProvider.getUriForFile(context,
                context.getApplicationContext().getPackageName() + ".fileprovider", file);

        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_SEND);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        ComponentName comp = new ComponentName("com.tencent.mm", "com.tencent.mm.ui.tools.ShareImgUI");
        intent.setComponent(comp);
        intent.setPackage(context.getPackageName());
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

        String type = "";
        for (int i = 0; i < TYPE_ARRAY.length; i++) {
            if (file.getAbsolutePath().toString().contains(TYPE_ARRAY[i][0].toString())) {
                type = TYPE_ARRAY[i][1];
                break;
            }
        }
        intent.setType(type);
        intent.putExtra(Intent.EXTRA_STREAM, contentPath);

	ActivityOptions options = null;
        if (displayId != -1) {
            options = ActivityOptions.makeBasic();
            options.setLaunchDisplayId(displayId);
        }
        context.startActivity(intent, options.toBundle());
    }

    private static final String[][] TYPE_ARRAY = {
            {".gif", "image/gif"},
            {".bmp", "image/bmp"},
            {".gif", "image/gif"},
            {".jpeg", "image/jpeg"},
            {".jpg", "image/jpeg"},
            {".png", "image/png"},
            {".txt", "text/plain"},
            {".doc", "application/msword"},
            {".apk", "application/vnd.android.package-archive"},
            {".gtar", "application/x-gtar"},
            {".pps", "application/vnd.ms-powerpoint"},
            {".ppt", "application/vnd.ms-powerpoint"},
            {".rar", "application/x-rar-compressed"},
            {".tar", "application/x-tar"},
            {".tgz", "application/x-compressed"},
            {".gz", "application/x-gzip"},
            {".gif", "image/gif"},
            {".mpc", "application/vnd.mpohun.certificate"},
            {".gif", "image/gif"},
            {".pdf", "application/pdf"},
            {".gif", "image/gif"},
            {".wps", "application/vnd.ms-works"},
            {".zip", "application/zip"},
            {".z", "application/x-compress"},
            {".3gp", "video/3gpp"},
            {".asf", "video/x-ms-asf"},
            {".avi", "video/x-msvideo"},
            {".m4u", "video/vnd.mpegurl"},
            {".m4v", "video/x-m4v"},
            {".mov", "video/quicktime"},
            {".mp4", "video/mp4"},
            {".gif", "image/gif"},
            {".mpe", "video/mpeg"},
            {".mpeg", "video/mpeg"},
            {".mpg", "video/mpeg"},
            {".mpg4", "video/mp4"},
            {".m3u", "audio/x-mpegurl"},
            {".m4a", "audio/mp4a-latm"},
            {".m4b", "audio/mp4a-latm"},
            {".m4p", "audio/mp4a-latm"},
            {".mp2", "audio/x-mpeg"},
            {".mp3", "audio/x-mpeg"},
            {".rmvb", "audio/x-pn-realaudio"},
            {".mpga", "audio/mpeg"},
            {".ogg", "audio/ogg"},
            {".wav", "audio/x-wav"},
            {".wma", "audio/x-ms-wma"},
            {".wmv", "audio/x-ms-wmv"},
            {"", "*/*"}
    };
}
