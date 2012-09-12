/*
 * AOA 2.0 implementation for Arduino (with USB Host Shiled library V1)
 * Copyright (C) 2012 Yuuichi Akagawa
 *
 * Original copyright
 *  Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __AndroidAccessory2_h__
#define __AndroidAccessory2_h__

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define ACCESSORY_AUDIO_NONE   0
#define ACCESSORY_AUDIO_ENABLE 1

class AndroidAccessory {
private:
    const char *manufacturer;
    const char *model;
    const char *description;
    const char *version;
    const char *uri;
    const char *serial;

    MAX3421E max;
    USB usb;
    bool connected;
    uint8_t in;
    uint8_t out;

    bool firstTime; //Connection timing indicator
    int protocol;   //AOA protocol version
    uint8_t audioMode;
    bool noApp;     //with/without App flag

    EP_RECORD epRecord[8];

    uint8_t descBuff[256];

    bool isAccessoryDevice(USB_DEVICE_DESCRIPTOR *desc)
    {
        return desc->idVendor == 0x18d1 &&
//            (desc->idProduct == 0x2D00 || desc->idProduct == 0x2D01);
            (desc->idProduct >= 0x2D00 && desc->idProduct <= 0x2D05);
    }

    int getProtocol(byte addr);
    void sendString(byte addr, int index, const char *str);
    bool switchDevice(byte addr);
    bool findEndpoints(byte addr, EP_RECORD *inEp, EP_RECORD *outEp);
    bool configureAndroid(void);

    //AOA2.0
    bool setAudioMode(uint8_t mode);

public:
    AndroidAccessory(const char *manufacturer,
                     const char *model,
                     const char *description,
                     const char *version,
                     const char *uri,
                     const char *serial,
                     uint8_t audiomode = ACCESSORY_AUDIO_NONE,
                     bool noapp = false);

    void powerOn(void);

    bool isConnected(void);
    int read(void *buff, int len, unsigned int nakLimit = USB_NAK_LIMIT);
    int write(void *buff, int len);

    int getProtocolVersion(void){return protocol;}

    //AOA2.0
    //for HID support
    bool hidRegist(uint8_t id , unsigned short descsize);
    bool hidUnRegist(uint8_t id);
    bool hidSetReportDesc(uint8_t id, char* desc, int len);
    bool hidSendEvent(uint8_t id, char* event, int len);
    bool isConnectedHID(uint8_t id, char *desc, int len);
    bool inline isFirstTime(void){return firstTime;}
    void clearFirstTime(void);
};

#endif /* __AndroidAccessory2_h__ */
