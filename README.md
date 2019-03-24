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
Below are the findings regarding the internal works of the mentioned app. This might help You reverse-engineer similar apps from Tuya. I've also marked some parts of the findings I believe are good (:+1:) or bad (:-1:) security practices.

### Make app debuggable
The app has simple mechanism to detect if it was repackaged - it checks the hash of certificate it was signed with (:-1: because it's easy to bypass) The class responsible for computing the hash resides in `smali_classes3/com/tuya/smart/common/oq.smali`.

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
The string to sign is prepared in Java and the algorithm is similar to this described in docs. The source can be viewed at `smali_classes3/com/tuya/smart/common/ok.smali`.

Sign is generated in native library (`libjnimain.so`). This lib exports only one function to Java (registered dynamically, not exported as symbol). I believe this is intentional to make reverse-engineering harder (:+1:)

```java
public static native Object doCommandNative(Context var0, int var1, byte[] var2, byte[] var3, byte[] var4, byte[] var5);
```

The `var1` is the actual function to run:
* 0 -> `init(appSecret, clientId, contents_of_bmp_file, certificate_sha256)`
* 1 -> `sign(strToSign, null, null, null)` returns signature as `java.lang.String`
* 2 -> *some crypto algorithms connected with MQTT, did not check exactly*

Even if there is no `doCommandNative` symbol exported, there are various other symbols present (see `libjnimain.so.symbols.txt` :-1:), especially some some crypto symbols (like: `mbedcrypto_md_hmac_starts`, `mbedcrypto_sha256_starts` which hints which algos are used to compute sign). Setting breakpoints on these functions revealed which algorithm and what input parameters were used for signature generation. (:-1:).

**Learning the key used for HMAC-SHA256 algorithm means we can write our own implementation of the signature algorithm**. Nevertheless, I was curious how the data was stored in BMP file. I've reverse-engineered the native ARM code (fortunately one of the exported symbols is named `read_keys_from_content` so You know where to look at :-1:) and written my own BMP file parser (see below).

## TestApp
Interesting parts of the code were reimplemented in simple Android app to ease up the debugging and analysis. It's possible to use the `libjnimain.so` in Your app and pass desired params to debug the native code consistently using `gdb`.

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
This is by far the most interesting part of the crypto riddle the Tuya gave us.

The key is encoded in BMP file with seemingly random pixels. It turns out Tuya's algorithm does not use standard image stenography approach (storing at most 1-2 bits of information in single pixel which would allow You to hide the token in "normal" image :-1:). Instead they store the whole series of bytes next to each other (the offsets in file are computed from hashed clientId value :+1: - You can see the details on the code). I've made a simple program to extract only the used bytes, You can see the original and only-used part next to each other below (they are enlarged):

<img src="/read-keys-from-bmp/test.bmp" alt="key file" width="400" alt="original">
<img src="/read-keys-from-bmp/used_pixels.bmp" alt="key file" width="400" alt="only used pixels">

This is where things start to look interesting/strange. The bytes read are not the parts of the final key, they are actually pairs of values ![(a_i, b_i)](/doc/coeffs.gif) (one being 6 and other 20 bytes long) which then are being used to create a N x N+1 matrix:

![\begin{bmatrix}
 (a_1)^3 & (a_1)^2 & a_1 & 1 &| &b_1 \\
 (a_2)^3 & (a_2)^2 & a_2 & 1 &| &b_2 \\
 (a_3)^3 & (a_3)^2 & a_3 & 1 &| &b_3 \\
 (a_4)^3 & (a_4)^2 & a_4 & 1 &| &b_4 \\
\end{bmatrix}](/doc/matrix.gif)

Then they use Linear Algebra to reduce the matrix to triangular form:

![\begin{bmatrix}
 r_1_1 & r_1_2 & r_1_3 & r_1_4 &| & c_1 \\
 0     & r_2_2 & r_2_3 & r_2_4 &| & c_2 \\
 0     & 0     & r_3_3 & r_3_4 &| & c_3 \\
 0     & 0     & 0     & r_4_4 &| & c_4 \\
\end{bmatrix}](/doc/matrix_triangle.gif)

The final solution is the division ![\frac{c_4}{r_4_4}](/doc/div.gif) - which should reduce to integer - interpreted as a hex string and converted to chars.

The code is quite complex which makes it hard to reverse engineer (:+1:), apart from that I don't see any reason to use linear algebra in here (maybe software developer tasked to implement the crypto really liked matrices :)).

Fortunately as the integer type cannot hold such a big values - and we wanted rational numeric mathematic - Tuya used free big-int library called [imath](https://github.com/creachadair/imath) and left their functions as dynamic symbols making it way easier to understand the code (:-1:).

I've provided the implementation of the above algorithm making it easy to extract the keys just from the disassembled APK file. The BMP file used in TuyaSmart app is in `assets/t_s.bmp` (to make it a *little* bit harder to find, they encoded the filename in base64 in Java source):
```java
return new String(Base64.decodeBase64((byte[])"dF9zLmJtcA==".getBytes()));
```
