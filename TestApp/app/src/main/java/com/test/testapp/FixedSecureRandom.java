package com.test.testapp;

import java.security.SecureRandom;

public class FixedSecureRandom
        extends SecureRandom {
    private static final long serialVersionUID = -144076469992183815L;
    byte[] seed = new byte[]{-86, -3, 18, -10, 89, -54, -26, 52, -119, -76, 121, -27, 7, 109, -34, -62, -16, 108, -75, -113};

    @Override
    public void nextBytes(byte[] arrby) {
        int n2 = 0;
        while (this.seed.length + n2 < arrby.length) {
            System.arraycopy(this.seed, 0, arrby, n2, this.seed.length);
            n2 += this.seed.length;
        }
        System.arraycopy(this.seed, 0, arrby, n2, arrby.length - n2);
    }
}
