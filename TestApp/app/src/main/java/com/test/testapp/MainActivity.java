package com.test.testapp;

import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;

import android.util.Log;
import android.widget.EditText;

public class MainActivity extends AppCompatActivity {
    public static final String req = "a=tuya.p.time.get||appVersion=3.8.0||clientId=3fjrekuxank9eaej3gcx||deviceId=ba0a0b82ce53c9f84863cc4bbc356ab7911eb3e4d89a||et=0.0.1||lang=pl||os=Android||requestId=281dba8f-0453-4158-9102-314a5b0e3f6c||time=1551625314||ttid=tuya||v=1.0";
    public static final String validSign = "d719494f223603c705ab797726588c771c9fd1c956351fb28bd008572136a75c";



    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);


        String sign = PkgSignTests.getHashedSign(this);

        if (sign != null) {
            Log.w("TEST", "getHashedSign: " + sign);
            EditText et = findViewById(R.id.text1);
            et.setText(sign);
        }

        final JNIWrapper jw;
        jw = new JNIWrapper(this);
        /*
        jw.init();

        Log.w("MAIN", "After init");

        Log.w("MAIN", "req: " + req);
        String sign = jw.getSign(req);
        Log.w("MAIN", "sign: " + sign);

        if (sign.equals(validSign)) {
            Log.w("MAIN", "SIGNATURE IS VALID");
        }
        */

        final RSATests rtest = new RSATests();

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                jw.init();
                try {
                    rtest.encrypt("asdf");
                } catch (Exception e) {
                    Log.w("MAIN", e);
                }


                String sign = jw.getSign(req);
                Log.w("MAIN", "sign: " + sign);
                String txt = "invalid";
                if (sign.equals(validSign)) {
                    txt = "SUCCESS";
                }


                Snackbar.make(view, "SIGN: " + txt, Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
}
