package com.test.testapp;

import android.content.Context;
import com.tuya.smart.security.jni.JNICLibrary;

import java.io.IOException;
import java.io.InputStream;

public class JNIWrapper {

    private final Context ctx;
    public static String mAppSecret = "aq7xvqcyqcnegvew793pqjmhv77rneqc";
    public static String mAppId = "3fjrekuxank9eaej3gcx";


    public JNIWrapper(Context context) {
        this.ctx = context;
    }

/*
    public static JNIWrapper getInstance() {
        if (b == null) {
            synchronized (JNIWrapper.class) {
                if (b == null) {
                    b = new JNIWrapper(TuyaSmartNetWork.getAppContext());
                }
            }
        }
        return b;
    }
    */

    public String getSign(String string2) {
        String string3;
        string2 = string3 = (String) JNICLibrary.doCommandNative(this.ctx, (int)1, (byte[])string2.getBytes(), (byte[])null, (byte[])null, (byte[])null);
        if (string3 == null) {
            string2 = "value == null";
        }
        return string2;
    }


    public String init() {
        String hash = "93:21:9F:C2:73:E2:20:0F:4A:DE:E5:F7:19:1D:C6:56:BA:2A:2D:7B:2F:F5:D2:4C:D5:5C:4B:61:55:00:1E:40";
        Object res = JNICLibrary.doCommandNative(this.ctx, 0, this.mAppSecret.getBytes(), this.mAppId.getBytes(), this.getFileContents(), (byte[])hash.getBytes());
        return (String)res;
    }

    private byte[] getFileContents() {
        try {
            InputStream is = this.ctx.getApplicationContext().getAssets().open("t_s.bmp");
            byte[] arrby = new byte[is.available()];
            is.read(arrby);
            is.close();
            return arrby;
        } catch (IOException iOException) {
            iOException.printStackTrace();
            return "asdoifpjqowiejroirprei".getBytes();
        }
    }
}
