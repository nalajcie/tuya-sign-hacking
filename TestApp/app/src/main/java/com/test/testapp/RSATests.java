package com.test.testapp;

import android.util.Log;

import java.lang.String;
import java.math.BigInteger;
import java.security.KeyFactory;
import java.security.PublicKey;
import java.security.SecureRandom;
import java.security.spec.RSAPublicKeySpec;
import javax.crypto.Cipher;

public class RSATests {
    public static final String TAG = "RSATests";


    public static String bytesToHexString(byte[] arrby) {
        StringBuilder stringBuilder = new StringBuilder("");
        if (arrby != null && arrby.length > 0) {
            for (int i2 = 0; i2 < arrby.length; ++i2) {
                String string2 = Integer.toHexString(arrby[i2] & 255);
                if (string2.length() < 2) {
                    stringBuilder.append(0);
                }
                stringBuilder.append(string2);
            }
            return stringBuilder.toString();
        }
        return null;
    }


    // checking public key generation implementation
    public static final String pk = "97989758108289813536701445960831424735858377215910329692856632329832587820641285276187676017060778739302769147142037170710235830237799692667335641891451934857211432782692473995865107749556252971051394233201134808074594477596175407856921717730764569302193111764878820655069455080663534513496637596883548793251";
    public static final String xx = "17d4f71e75eed9cfbaf0036b48836684"; // md5(passwd)

    public String encrypt(String args) throws Exception{
        byte[] input_arr = xx.getBytes();
        PublicKey pubKey = KeyFactory.getInstance("RSA").generatePublic(new RSAPublicKeySpec(new BigInteger(pk), new BigInteger("3")));
        Cipher cipher = Cipher.getInstance("RSA"); // WARN: Android returns different implementation when using "RSA" type - RSA without padding (!!!)
        FixedSecureRandom fixedSecureRandom = new FixedSecureRandom();

        cipher.init(1, pubKey, (SecureRandom) fixedSecureRandom);


        Log.d(TAG, "getBlockSize: " + cipher.getBlockSize()); // this returns 127
        Log.d(TAG, "getOutputSize: "  + cipher.getOutputSize(input_arr.length)); // this returns 128

        int blockSize = cipher.getBlockSize();
        int n2 = cipher.getOutputSize(input_arr.length);
        int blocksCnt = (input_arr.length % blockSize != 0 ? input_arr.length / blockSize + 1 : input_arr.length / blockSize);

        Log.d(TAG, "blocksCnt=" + blocksCnt);

        byte[] output_arr = new byte[blocksCnt * n2];
        blocksCnt = 0;

        do {
            Log.d(TAG, "[" + blocksCnt + "]: " + bytesToHexString(output_arr));
            block6 : {
                int n5 = input_arr.length;
                int n6 = blocksCnt * blockSize;
                if (n5 - n6 <= 0) break;
                if (input_arr.length - n6 > blockSize) {

                    cipher.doFinal(input_arr, n6, blockSize, output_arr, blocksCnt * n2);
                    break block6;
                }
                //doFinal(byte[] input, int inputOffset, int inputLen, byte[] output, int outputOffset)
                cipher.doFinal(input_arr, n6, input_arr.length - n6, output_arr, blocksCnt * n2);
            }
            ++blocksCnt;
        } while (true);

        Log.d(TAG, "RESULT: " + bytesToHexString(output_arr));
        return bytesToHexString(output_arr);
    }
}