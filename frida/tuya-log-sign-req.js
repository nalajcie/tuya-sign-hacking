function dumpJavaBytes(v) {
    var buffer = Java.array('byte', v);
    var result = "";
    for(var i = 0; i < buffer.length; ++i){
        result+= (String.fromCharCode(buffer[i]));
    }
    return result;
}

setImmediate(function() {
    console.log("[*] Starting TUYA script");
    Java.perform(function() {
        /*
        // this does not work consistently under frida, use overrides below
        orClass = Java.use("com.tuya.smart.common.or");
        orClass.a.overload('java.lang.String').implementation = function(v) {
            console.log("[*] SIGN called");
            sign = this.a(v);
            console.log("[*]  => str to sign: ", v);
            console.log("[*]  => signature : ", sign);
            return "asdf"; //sign;
        };
        console.log("[*] SIGN handler modified");
        */

        // log sign requests
        jniClass = Java.use("com.tuya.smart.security.jni.JNICLibrary");
        jniClass.doCommandNative.implementation = function(ctx, cmd, v2, v3, v4, v5) {
            //console.log("doCommandNative(%d)", cmd);
            ret = this.doCommandNative(ctx, cmd, v2, v3, v4, v5);
            if (cmd == 1) {
                console.log("SIGN: ", dumpJavaBytes(v2));
                console.log("RET: ", ret);
            }

            return ret;
        };


        // log GET and POST data
        apiParamsClass = Java.use("com.tuya.smart.android.network.TuyaApiParams");
        apiParamsClass.getRequestUrl.overload().implementation = function() {
            res = this.getRequestUrl();
            console.log("URL: ", res);
            postData = this.getPostDataString();
            console.log("POST: ", postData);
            return res;
        };

        console.log("[*] getRequestUrl modified");


        // log JSON responses
        logClass = Java.use("com.tuya.smart.android.common.utils.L");
        logClass.logJson.implementation = function(tag, json) {
            console.log("JSON: ", json);
        };


        // log MD5 requests
        md5Class = Java.use("com.tuya.smart.android.common.utils.MD5Util");
        md5Class.md5AsBase64.overload('java.lang.String').implementation = function(input) {
            console.log("MD5 in: ", input);
            res = this.md5AsBase64(input);
            console.log("MD5 out: ", res);
            return res;
        }

        // debuggable
        // com.tuyasmart.sample.app.TuyaSmartApplication

    });

});

