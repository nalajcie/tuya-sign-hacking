package com.tuya.smart.security.jni;

import android.content.Context;

public class JNICLibrary {
    static {
        try {
            System.loadLibrary("c++_shared");
            System.loadLibrary("jnimain");
        }
        catch (Throwable throwable) {
            throwable.printStackTrace();
        }
    }

    public static native Object doCommandNative(Context var0, int var1, byte[] var2, byte[] var3, byte[] var4, byte[] var5);
}


