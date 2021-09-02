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

import java.net.*;
import java.io.*;

public class VsockClientImpl {
    static {
        System.loadLibrary("VsocketClientImpl");
    }

    int fd = -1;

    void create() throws SocketException {
        socketCreate();
    }

    native void socketCreate() throws SocketException;
    native void connect(VsockAddress address) throws SocketException;
    native void close() throws IOException;
    native void write(byte[] b, int off, int len) throws IOException;
    native int read(byte[] b, int off, int len) throws IOException;

    native void writeInt(int value) throws IOException;
    native int readInt() throws IOException;
}
