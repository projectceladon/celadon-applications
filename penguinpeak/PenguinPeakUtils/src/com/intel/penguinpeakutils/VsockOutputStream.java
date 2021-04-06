package com.intel.penguinpeakutils;

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

    @Override
    public void close() throws IOException {
        vSock.close();
        super.close();
    }
}
