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

import java.net.SocketAddress;
import java.util.Objects;

public final class VsockAddress extends SocketAddress {
    public static final int VMADDR_CID_ANY = -1;
    public static final int VMADDR_CID_HYPERVISOR = 0;
    public static final int VMADDR_CID_RESERVED = 1;
    public static final int VMADDR_CID_HOST = 2;
    public static final int VMADDR_CID_PARENT = 3;

    public static final int VMADDR_PORT_ANY = -1;
    final int cid;
    final int port;

    public VsockAddress(int cid, int port) {
        this.cid = cid;
        this.port = port;
    }

    public int getCid() {
        return cid;
    }

    public int getPort() {
        return port;
    }
}
