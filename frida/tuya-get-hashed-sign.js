setImmediate(function() {
    console.log("[*] Starting TUYA script");
    Java.perform(function() {
        oqClass = Java.use("com.tuya.smart.common.oq");
        oqClass.a.overload('android.content.Context').implementation = function(v) {
            const correctVal = "93:21:9F:C2:73:E2:20:0F:4A:DE:E5:F7:19:1D:C6:56:BA:2A:2D:7B:2F:F5:D2:4C:D5:5C:4B:61:55:00:1E:40"

            console.log("[*] getHashedSign called");
            retval = this.a(v);
            console.log("[*]  => returns: ", retval);
            console.log("[*]  => we ret : ", correctVal);
            return correctVal;
        };

        console.log("[*] getHasedSign handler modified");

        // debuggable
        // com.tuyasmart.sample.app.TuyaSmartApplication
/*
        tsaClass = Java.use("com.tuyasmart.sample.app.TuyaSmartApplication");
        tsaClass.isBuildConfigDebug.implementation = function(v) {
            console.log("[*] isBuildConfigDebug called");

            return true;
        };
    */

    });

});

