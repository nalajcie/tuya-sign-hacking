# Frida scripts
These scripts allow You to log internal flow of the TuyaSmart App, especially:
* sign requests - input params and the sign result
* HTTPS requests - URL + data sent
* HTTPS responses - data received

The HTTPS flow can be currently intercepted with HTTPS proxy (eg. Anyproxy), because Tuya does not use Certificate Pinnig (:-1:). Even so, using `frida` is more future-proof.

## Usage
```bash
frida-ps -U
frida -U -l tuya-get-hashed-sign.js com.tuya.smart
frida -U -l tuya-log-sign-req.js com.tuya.smart
```
