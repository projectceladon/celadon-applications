package com.intel.penguinpeakutils;

import java.io.IOException;
import java.io.InputStream;

public final class VsockInputStream extends InputStream {
    private final VsockClientImpl vSock;
    private byte[] temp;

    public VsockInputStream (VsockClientImpl vSock) {
        this.vSock = vSock;
    }

    @Override
    public int read(byte[] b, int off, int len) throws IOException {
        return vSock.read(b, off, len);
    }

    @Override
    public int read() throws IOException {
        temp = new byte[1];
        int n = read(temp, 0, 1);
        if (n <= 0) {
            return -1;
        }
        return temp[0];
    }

    @Override
    public void close() throws IOException {
        vSock.close();
        super.close();
    }
}
