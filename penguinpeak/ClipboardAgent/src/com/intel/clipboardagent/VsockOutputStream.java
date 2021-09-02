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

import java.io.IOException;
import java.io.OutputStream;

public final class VsockOutputStream extends OutputStream {
    private final VsockClientImpl vSock;
    private final byte[] temp = new byte[1];

    VsockOutputStream(VsockClientImpl vSock) {
        this.vSock = vSock;
    }

    @Override
    public void write(int b) throws IOException {
        temp[0] = (byte) b;
        this.write(temp, 0, 1);
    }

    @Override
    public void write(byte[] b, int off, int len) throws IOException {
        vSock.write(b, off, len);
    }

    public void writeInt(int value) throws IOException {
        vSock.writeInt(value);
    }

    @Override
    public void close() throws IOException {
        vSock.close();
        super.close();
    }
}
