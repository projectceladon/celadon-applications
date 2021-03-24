package com.intel.penguinpeakutils;

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
