package com.test.testapp;

import android.app.Application;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.util.Log;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.cert.Certificate;
import java.security.cert.CertificateEncodingException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;


public class PkgSignTests {

    /* This is the certificate hash function Tuya uses */
    public static String getHashedSign(Context ctx) {
        PackageManager pm = ctx.getPackageManager();
        //String pkgname = "com.test.testapp";
        String pkgname = "com.tuya.smart";
        PackageInfo pi;
        try {
            pi = pm.getPackageInfo(pkgname, PackageManager.GET_SIGNATURES);
        }
        catch (PackageManager.NameNotFoundException nameNotFoundException) {
            nameNotFoundException.printStackTrace();
            return "";
        }
        Log.w("PkgInfo", pi.toString());
        InputStream sign = new ByteArrayInputStream(pi.signatures[0].toByteArray());
        Log.w("sign", sign.toString());

        CertificateFactory cf;
        try {
            cf = CertificateFactory.getInstance("X509");
        }
        catch (Exception exception) {
            exception.printStackTrace();
            return "";
        }
        X509Certificate cert;
        try {
            cert = (X509Certificate) cf.generateCertificate(sign);
        }
        catch (Exception exception) {
            exception.printStackTrace();
            return "";
        }
        // this is the certificate the application was sighed with
        Log.w("cert", cert.toString());

        byte[] sha = {'f', 'a', 'i', 'l', '\0'};

        try {
            sha = MessageDigest.getInstance("SHA256").digest(cert.getEncoded());
        }
        catch (CertificateEncodingException certificateEncodingException) {
            certificateEncodingException.printStackTrace();
        }
        catch (NoSuchAlgorithmException noSuchAlgorithmException) {
            noSuchAlgorithmException.printStackTrace();
        }
        //Log.w("SHA256 of the cert", sha.toString());

        return PkgSignTests.shaToStr(sha);
    }

    private static String shaToStr(byte[] arrby) {
        StringBuilder stringBuilder = new StringBuilder(arrby.length * 2);
        for (int i2 = 0; i2 < arrby.length; ++i2) {
            String string2 = Integer.toHexString(arrby[i2]);
            int n2 = string2.length();
            if (n2 == 1) {
                string2 = "0" + string2;
            }
            if (n2 > 2) {
                string2 = string2.substring(n2 - 2, n2);
            }
            stringBuilder.append(string2.toUpperCase());
            if (i2 >= arrby.length - 1) continue;
            stringBuilder.append(':');
        }
        return stringBuilder.toString();
    }
}
