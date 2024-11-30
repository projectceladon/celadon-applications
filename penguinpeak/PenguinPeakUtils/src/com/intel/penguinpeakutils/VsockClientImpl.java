package com.intel.penguinpeakutils;

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
}
