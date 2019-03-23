# tuya-sign-hacking
Tools for reverse-engineering and description of the new TUYA API sign algorithm..

## Motivation
TUYA cheap IoT products are widely available under different names and brands on the market. You can control them using Tuya mobile apps (Tuya Smart, Smart Life, and some branded versions). Because of the business model of TUYA, they provide separate namespaces for separate apps (If You register Your device in one app, it won't be visible int the other unless they share the `clientId`).

I would like to be able to control/view state of my devices from other services (push data to infuxdb/grafana, control using domoticz, etc.), especially get data from Smart Power Plug with integrated power monitor.
As there is an [open API to get data and command devices from the cloud](https://docs.tuya.com/en/openapi/index.html) it should be possible to extract `clientId` and `appSecret` from the original app and use it within the API.

Unfortunately that doesn't work - You would either receive signature error or prompt to upgrade the mobile app. I suppose they obsoleted some parts of the described API for particular `clinetId`'s. The only way to obtain the data and still use the original TUYA app is to learn how the new API - which the app uses - works.

This repository contains the summary of my findings and the tools I've used in the process, based on **TuyaSmart App for Android v. 3.8.0**.

## TL;DR
For the impatient:
* the data to sign is constructed the same way it is described in current open API
* new API signature uses `HMAC-SHA256` algorithm with secret key in format: `[application certificate SHA256]_[secret token hidden in bmp file]_[appSecret]` (below are the details on how to obtain each of the part)
* the secret key value for TuyaSmart App is `93:21:9F:C2:73:E2:20:0F:4A:DE:E5:F7:19:1D:C6:56:BA:2A:2D:7B:2F:F5:D2:4C:D5:5C:4B:61:55:00:1E:40_vay9g59g9g99qf3rtqptmc3emhkanwkx_aq7xvqcyqcnegvew793pqjmhv77rneqc`
* You need to use enhanced login process described in the [docs](https://docs.tuya.com/en/cloudapi/appAPI/userAPI/tuya.m.user.email.password.login_1.0.html) (using the empheral RSA public key to encrypt the MD5(passwd))


# Reverse-engineering Android App

## Intro
I haven't done any reverse engineering on Android previously, so I needed a crash course on available tools and methods.

### Docs
If You would like to dig into this field, I would strongly recommend the following resources:

* the [The Mobile Security Testing Guide](https://github.com/OWASP/owasp-mstg) from the Open Web Application Security Project (OWASP) Foundation
* ...especially  the [Tampering and Reverse Engineering on Android](https://github.com/OWASP/owasp-mstg/blob/master/Document/0x05c-Reverse-Engineering-and-Tampering.md)
* [Docker image](https://github.com/cryptax/androidre) with software for reverse engineering Android Apps

### Tools
I've used standard tools described in the docs mentioned above, I'll not dig into how to install/use them:

* `apktool` to decode/build (reassemble) the APK file
* `apkx` to decompile smali into Java (for ease of static analysis, not always correct)
* IntelliJ IDE with smalidea plugin - which allows You to debug smali code on target (!)
* `gdb` (wrapped in `ndk-gdb` python script) to debug native shared libraries on target
* [`frida`](https://www.frida.re/) - dynamic instrumentation toolkit (allows You to trace/overwrite native/Java methods in runtime - without the need to recompile/reassemble the APK) - I really love it :)
* [`ghidra`](https://ghidra-sre.org/) - new free disassembler/decompiler from the NSA (better than IDA Pro, because it's free - even if it sends the codes to the NSA :P) - used to reverse-engineer the `libjnimain.so`
* android sdk to build simple `TestApp` with code snippets taken from original app to test them out

## TuyaSmart App v3.8.0
Below are the findings regarding the internal works of the mentioned app. This might help You reverse-engineer similar apps from Tuya.

### Make app debuggable
The app has single mechanism to detect if it was repackaged - it checks the hash of certificate it was signed with. The class responsible for computing the hash resides in `smali_classes3/com/tuya/smart/common/oq.smali`.

The computed hash is then passed to native library using call `doCommandNative`.

To return the original hash, just add code like this at the beginning of the `a()` method:
```smali
const-string p0, "93:21:9F:C2:73:E2:20:0F:4A:DE:E5:F7:19:1D:C6:56:BA:2A:2D:7B:2F:F5:D2:4C:D5:5C:4B:61:55:00:1E:40"
return-object p0
```

The details on how the hash is computed and where to obtain it are in the section below.

* Decode the APK (using apktool)
* change `AndroidManifest.xml` to include `android:debuggable="true"`
* change the `smali_classes3/com/tuya/smart/common/oq.smali` as described above
* repack the APK (using apktool)
* zipalign/sign and install to target

Apart from the change above You can Also try to enable verbose logging (The app will output HTTP requests/responses to android log) by changing the output of internal `isDebug` functions, eg:
* function `isBuildConfigDebug` at `smali/com/tuyasmart/sample/app/TuyaSmartApplication.smali`
* function `isBuildConfigDebug` at `smali/com/tuyasmart/stencil/app/StencilApp.smali`

### Internal works
The string to sign is prepared in Java and the algorithm is similar to this described in docs. 


# "Secret Key" components

## clientId/appSecret
This one is pretty straightforward, as it's written in plaintext in the App source. You can find them in `smali/com/tuyasmart/sample/app/TuyaSmartApplication.smali` function `initKey`:
```smali
const-string v2, "3fjrekuxank9eaej3gcx"
const-string v3, "aq7xvqcyqcnegvew793pqjmhv77rneqc"
```
The code indicates it's also possible to store them in the app preferences (that might be used to produce the branded apps easily?) and that it will output these values to the android log if it's debug build). It's also possible to intercept these values using `frida`.
 
## Application certificate SHA256
The class responsible for computing the hash resides in `smali_classes3/com/tuya/smart/common/oq.smali`.
To obtain the correct hash for a given app, You can either use TestApp (which has the same algorithm implemented in Java) or get it from decoded APK:

```bash
openssl pkcs7 -inform DER -print_certs -in CERT.RSA -out cert.pem
openssl x509 -in cert.pem -outform der | sha256sum | tr a-f A-F | sed 's/.\{2\}/&:/g' | cut -c 1-95
```

## Secret token from the BMP file

