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

import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.SocketException;

public final class Vsock extends VsockBaseVSock implements Closeable {
    private boolean connected = false;
    private VsockOutputStream outputStream;
    private VsockInputStream inputStream;
    private VsockAddress mAddress;

    public Vsock() {
    }

    public Vsock(VsockAddress address) {
       mAddress = address;
    }

    public void connect() throws SocketException {
        if (isClosed()) {
            throw new SocketException("Socket closed");
        }
        if (connected) {
            throw new SocketException("Socket already connected");
        }
        getImplementation().connect(mAddress);
        connected = true;
    }

    public synchronized VsockOutputStream getOutputStream() throws IOException {
        if (isClosed()) {
            throw new SocketException("VSock is closed");
        }
        if (outputStream == null) {
            outputStream = new VsockOutputStream(getImplementation());
        }
        return outputStream;
    }

    public synchronized VsockInputStream getInputStream() throws IOException {
        if (isClosed()) {
            throw new SocketException("VSock is closed");
        }
        if (inputStream == null) {
            inputStream = new VsockInputStream(getImplementation());
        }
        return inputStream;
    }

    void postAccept() {
        created = true;
        connected = true;
    }
}
