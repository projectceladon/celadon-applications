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
import java.net.SocketException;

abstract class VsockBaseVSock implements Closeable {
    protected final Object closeLock = new Object();
    protected boolean closed = false;
    protected boolean created = false;

    private VsockClientImpl implementation;

    private void createImplementation() throws SocketException {
        implementation = new VsockClientImpl();
        implementation.create();
        created = true;
    }

    protected VsockClientImpl getImplementation() throws SocketException {
        if (!created) {
            createImplementation();
        }
        return implementation;
    }

    protected VsockClientImpl setImplementation() throws SocketException {
        if(implementation == null) {
            implementation = new VsockClientImpl();
        }
        return implementation;
    }

    @Override
    public synchronized void close() throws IOException {
        synchronized (closeLock) {
            if (isClosed())
                return;
            if (created)
                getImplementation().close();
            closed = true;
        }
    }

    protected boolean isClosed() {
        return closed;
    }
}
