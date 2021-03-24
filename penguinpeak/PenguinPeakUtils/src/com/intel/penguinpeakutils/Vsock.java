package com.intel.penguinpeakutils;

import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.SocketException;

public final class Vsock extends VsockBaseVSock implements Closeable {
    private boolean connected = false;
    private VsockOutputStream outputStream;
    private VsockInputStream inputStream;

    public Vsock() {
    }

    public Vsock(VsockAddress address) {
        try {
            getImplementation().connect(address);
        } catch (Exception e) {
            try {
                close();
            } catch (Exception ce) {
                e.addSuppressed(ce);
            }
            throw new IllegalStateException(e.getMessage(), e);
        }
    }

    public void connect(VsockAddress address) throws SocketException {
        if (isClosed()) {
            throw new SocketException("Socket closed");
        }
        if (connected) {
            throw new SocketException("Socket already connected");
        }
        getImplementation().connect(address);
        connected = true;
    }

    public synchronized OutputStream getOutputStream() throws IOException {
        if (isClosed()) {
            throw new SocketException("VSock is closed");
        }
        if (outputStream == null) {
            outputStream = new VsockOutputStream(getImplementation());
        }
        return outputStream;
    }

    public synchronized InputStream getInputStream() throws IOException {
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
