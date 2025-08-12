## Bluetooth LE HID Media Controller 
Unfinished example mainly meant to be used a reference for different projects.

#### Specifications:
- Made for ESP32
- Using Apache Mynewt NimBLE stack
- Uses HID over GATT service - this just means BLE HID (Currently I've only implemented Play/Pause and Volume Control)

#### Requirements:
- Make sure to enable Bluetooth with NimBLE as the Host in the config.
- Make sure to enable "Persit the BLE Bonding keys in NVS"
- If the report descriptor (hid_descriptor_report) is changed, update the LEN_HID_DESCRIPTOR_REPORT in "hid_vars.h"

#### Useful Documentation/Sources:
- HID Usage Table: https://usb.org/sites/default/files/hut1_6.pdf (may be out of date but worked for what I wanted)
- HID Class Definition: https://www.usb.org/sites/default/files/hid1_11.pdf (may be out of date but should contain everything you need to understand HID variables)
- Bluetooth Characteristic UUIDs: https://btprodspecificationrefs.blob.core.windows.net/assigned-numbers/Assigned%20Number%20Types/Assigned_Numbers.pdf
- Apache NimBLE: https://mynewt.apache.org/latest/network/ble_hs/ble_hs.html (gives a rough overview on the stack, mainly useful for error codes)

##### Tips:
- It may be overwhelming at first so go over code slowly and have a proper look at the libraries used.
- Look through the nimBLE examples that come with Esp
