from ctypes import *


testdll = cdll.LoadLibrary("get-parent-device.dll")
print(testdll)
print(vars(testdll))
print(testdll.tprint())
print(testdll.process_func(r'USBSTOR\DISK&VEN_MBED&PROD_VFS&REV_0.1\7&2B38C71E&0&0000000197969908A5A5A5A5066EFF383930545043205430&0',r'.*\\ROOT_HUB.*'))