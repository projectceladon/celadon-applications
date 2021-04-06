package com.intel.penguinpeakutils;

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
