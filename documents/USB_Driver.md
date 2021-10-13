# USB Video Class 驱动调研

## libuvc

依赖于libusb，如libusb get device list等

Documents at https://int80k.com/libuvc/doc/

## libusb

**libusb** is a C library that provides generic access to USB devices. It is intended to be used by developers to facilitate the production of applications that communicate with USB hardware.

Source: https://github.com/libusb/libusb

##  uspi

是一个bare metal的USB库（包括自己开MMU啥的）

## 目前计划

还是需要在实际要支持的板子上（linux）看能不能从头编一个能够不依赖系统库支持摄像头的应用，然后再看能不能再将这个应用编到Chcore里。