# tuya-sign-hacking
Tools for reverse-engineering and description of the new TUYA API sign algorithm..

## Motivation
TUYA cheap IoT protucts are widely available under different names and brands on the market. You can control them using Tuya mobile apps (Tuya Smart, Smart Life, and some branded versions). Because of the business model of TUYA, they provide separate namespaces for separate apps (If You register Your device in one app, it won't be visible int the other unless they share the `clientId`).

I would like to be able to control/view state of my devices from other services (push data to infuxdb/grafana, control using domoticz, etc.), especially get data from Smart Power Plug with integrated power monitor.
As there is an [open API to get data and command devices from the cloud](https://docs.tuya.com/en/openapi/index.html) it should be possible to extract `clientId` and `appSecret` from the original app and use it within the API.

Unfortunately that doesn't work - You would either receive signature error or prompt to upgrade the mobile app. I suppose they obsoleted some parts of the described API for particular `clinetId`'s. The only way to obtain the data and still use the original TUYA app is to learn how the new API - which the app uses - works.

This repository contains the summary of my findigns and the tools I've used in the process, based on **TuyaSmart App for Android v. 3.8.0**.

## TL;DR
For the impatient:
* the data to sign is constructed the same way it is described in current open API
* new API signature uses `HMAC-SHA256` algorithm with secret key in format: `[application certificate MD hash]_[secret token hidden in bmp file]_[appSecret]` (below are the details on how to obtain each of the part)
* the secret key value for TuyaSmart App is `93:21:9F:C2:73:E2:20:0F:4A:DE:E5:F7:19:1D:C6:56:BA:2A:2D:7B:2F:F5:D2:4C:D5:5C:4B:61:55:00:1E:40_vay9g59g9g99qf3rtqptmc3emhkanwkx_aq7xvqcyqcnegvew793pqjmhv77rneqc`
* You need to use enhanced login process described in the [docs](https://docs.tuya.com/en/cloudapi/appAPI/userAPI/tuya.m.user.email.password.login_1.0.html) (using the empheral RSA public key to encrypt the MD5(passwd))


## Reverse-engineering Anddroid App

### Intro

### Tools used

## "Secret Key" components

### clientId/appSecret

### Application certificate MD hash

### Secret token from the BMP file

