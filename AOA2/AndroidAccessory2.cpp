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


#include <Max3421e.h>
#include <Usb.h>
#include <AndroidAccessory2.h>

//AOA1.0 Specific
#define USB_ACCESSORY_PRODUCT_ID        0x2D00
#define USB_ACCESSORY_ADB_PRODUCT_ID    0x2D01

//AOA2.0 Specific
#define USB_AUDIO_PRODUCT_ID                0x2D02
#define USB_AUDIO_ADB_PRODUCT_ID            0x2D03
#define USB_ACCESSORY_AUDIO_PRODUCT_ID      0x2D04
#define USB_ACCESSORY_AUDIO_ADB_PRODUCT_ID  0x2D05

#define ACCESSORY_STRING_MANUFACTURER   0
#define ACCESSORY_STRING_MODEL          1
#define ACCESSORY_STRING_DESCRIPTION    2
#define ACCESSORY_STRING_VERSION        3
#define ACCESSORY_STRING_URI            4
#define ACCESSORY_STRING_SERIAL         5

#define ACCESSORY_GET_PROTOCOL          51
#define ACCESSORY_SEND_STRING           52
#define ACCESSORY_START                 53

//AOA2.0 command
#define ACCESSORY_REGISTER_HID          54
#define ACCESSORY_UNREGISTER_HID        55
#define ACCESSORY_SET_HID_REPORT_DESC   56
#define ACCESSORY_SEND_HID_EVENT        57
#define SET_AUDIO_MODE                  58

AndroidAccessory::AndroidAccessory(const char *manufacturer,
                                   const char *model,
                                   const char *description,
                                   const char *version,
                                   const char *uri,
                                   const char *serial,
                                   uint8_t  audiomode,
                                   bool         noapp) : manufacturer(manufacturer),
                                                         model(model),
                                                         description(description),
                                                         version(version),
                                                         uri(uri),
                                                         serial(serial),
                                                         connected(false),
                                                         protocol(-1),
                                                         audioMode(audiomode),
                                                         noApp(noapp)
{

}

void AndroidAccessory::powerOn(void)
{
    max.powerOn();
    delay(200);
}

int AndroidAccessory::getProtocol(byte addr)
{
    uint16_t protocol = -1;
    usb.ctrlReq(addr, 0,
                USB_SETUP_DEVICE_TO_HOST |
                USB_SETUP_TYPE_VENDOR |
                USB_SETUP_RECIPIENT_DEVICE,
                ACCESSORY_GET_PROTOCOL, 0, 0, 0, 2, (char *)&protocol);
    return protocol;
}

void AndroidAccessory::sendString(byte addr, int index, const char *str)
{
    usb.ctrlReq(addr, 0,
                USB_SETUP_HOST_TO_DEVICE |
                USB_SETUP_TYPE_VENDOR |
                USB_SETUP_RECIPIENT_DEVICE,
                ACCESSORY_SEND_STRING, 0, 0, index,
                strlen(str) + 1, (char *)str);
}


bool AndroidAccessory::switchDevice(byte addr)
{
    protocol = getProtocol(addr);

    if (protocol >= 1) {
        Serial.print("device supports protocol 1 or higher\n");
    } else {
        Serial.print("could not read device protocol version\n");
        return false;
    }

    if(!(protocol >= 2 && noApp)){ //if AOA2.0 or higher, you can skip App
        sendString(addr, ACCESSORY_STRING_MANUFACTURER, manufacturer);
        sendString(addr, ACCESSORY_STRING_MODEL, model);
    }
    sendString(addr, ACCESSORY_STRING_DESCRIPTION, description);
    sendString(addr, ACCESSORY_STRING_VERSION, version);
    sendString(addr, ACCESSORY_STRING_URI, uri);
    sendString(addr, ACCESSORY_STRING_SERIAL, serial);

    //Audio Support
    if( protocol >= 2 && audioMode != ACCESSORY_AUDIO_NONE ){
        setAudioMode(audioMode);
    }

    usb.ctrlReq(addr, 0,
                USB_SETUP_HOST_TO_DEVICE |
                USB_SETUP_TYPE_VENDOR |
                USB_SETUP_RECIPIENT_DEVICE,
                ACCESSORY_START, 0, 0, 0, 0, NULL);

    while (usb.getUsbTaskState() != USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE) {
        max.Task();
        usb.Task();
    }

    return true;
}

// Finds the first bulk IN and bulk OUT endpoints
bool AndroidAccessory::findEndpoints(byte addr, EP_RECORD *inEp, EP_RECORD *outEp)
{
    int len;
    byte err;
    uint8_t *p;

    err = usb.getConfDescr(addr, 0, 4, 0, (char *)descBuff);
    if (err) {
        Serial.print("Can't get config descriptor length\n");
        return false;
    }


    len = descBuff[2] | ((int)descBuff[3] << 8);
    if (len > sizeof(descBuff)) {
        Serial.print("config descriptor too large\n");
            /* might want to truncate here */
        return false;
    }

    err = usb.getConfDescr(addr, 0, len, 0, (char *)descBuff);
    if (err) {
        Serial.print("Can't get config descriptor\n");
        return false;
    }

    p = descBuff;
    inEp->epAddr = 0;
    outEp->epAddr = 0;
    while (p < (descBuff + len)){
        uint8_t descLen = p[0];
        uint8_t descType = p[1];
        USB_ENDPOINT_DESCRIPTOR *epDesc;
        EP_RECORD *ep;

        switch (descType) {
        case USB_DESCRIPTOR_CONFIGURATION:
            Serial.print("config desc\n");
            break;

        case USB_DESCRIPTOR_INTERFACE:
            Serial.print("interface desc\n");
            break;

        case USB_DESCRIPTOR_ENDPOINT:
            epDesc = (USB_ENDPOINT_DESCRIPTOR *)p;
            if (!inEp->epAddr && (epDesc->bEndpointAddress & 0x80))
                ep = inEp;
            else if (!outEp->epAddr)
                ep = outEp;
            else
                ep = NULL;

            if (ep) {
                ep->epAddr = epDesc->bEndpointAddress & 0x7f;
                ep->Attr = epDesc->bmAttributes;
                ep->MaxPktSize = epDesc->wMaxPacketSize;
                ep->sndToggle = bmSNDTOG0;
                ep->rcvToggle = bmRCVTOG0;
            }
            break;

        default:
            Serial.print("unkown desc type ");
            Serial.println( descType, HEX);
            break;
        }

        p += descLen;
    }

    if (!(inEp->epAddr && outEp->epAddr))
        Serial.println("can't find accessory endpoints");

    return inEp->epAddr && outEp->epAddr;
}

bool AndroidAccessory::configureAndroid(void)
{
    byte err;
    EP_RECORD inEp, outEp;

    if (!findEndpoints(1, &inEp, &outEp))
        return false;

    memset(&epRecord, 0x0, sizeof(epRecord));

    epRecord[inEp.epAddr] = inEp;
    if (outEp.epAddr != inEp.epAddr)
        epRecord[outEp.epAddr] = outEp;

    in = inEp.epAddr;
    out = outEp.epAddr;

    Serial.println(inEp.epAddr, HEX);
    Serial.println(outEp.epAddr, HEX);

    epRecord[0] = *(usb.getDevTableEntry(0,0));
    usb.setDevTableEntry(1, epRecord);

    err = usb.setConf( 1, 0, 1 );
    if (err) {
        Serial.print("Can't set config to 1\n");
        return false;
    }

    usb.setUsbTaskState( USB_STATE_RUNNING );

    return true;
}

bool AndroidAccessory::isConnected(void)
{
    USB_DEVICE_DESCRIPTOR *devDesc = (USB_DEVICE_DESCRIPTOR *) descBuff;
    byte err;

    max.Task();
    usb.Task();

    if (!connected &&
        usb.getUsbTaskState() >= USB_STATE_CONFIGURING &&
        usb.getUsbTaskState() != USB_STATE_RUNNING) {
        Serial.print("\nDevice addressed... ");
        Serial.print("Requesting device descriptor.\n");

        err = usb.getDevDescr(1, 0, 0x12, (char *) devDesc);
        if (err) {
            Serial.print("\nDevice descriptor cannot be retrieved. Trying again\n");
            return false;
        }

        if (isAccessoryDevice(devDesc)) {
            Serial.print("found android acessory device\n");
            Serial.print("VID:");
            Serial.print(devDesc->idVendor, HEX);
            Serial.print(", PID:");
            Serial.println(devDesc->idProduct, HEX);

            connected = configureAndroid();
            firstTime = connected;
        } else {
            Serial.print("found possible device. swithcing to serial mode\n");
            switchDevice(1);
        }
    } else if (usb.getUsbTaskState() == USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE) {
        if (connected)
            Serial.println("disconnect\n");
        connected = false;
        firstTime = false;
    }
    return connected;
}

int AndroidAccessory::read(void *buff, int len, unsigned int nakLimit)
{
    return usb.newInTransfer(1, in, len, (char *)buff, nakLimit);
}

int AndroidAccessory::write(void *buff, int len)
{
    usb.outTransfer(1, out, len, (char *)buff);
    return len;
}

/*
 * AOA2.0 Supoort
 */
//mode=0: Audio off
//     1: Audio On
bool AndroidAccessory::setAudioMode(uint8_t mode)
{
    usb.ctrlReq(1, 0,
                USB_SETUP_HOST_TO_DEVICE |
                USB_SETUP_TYPE_VENDOR |
                USB_SETUP_RECIPIENT_DEVICE,
                SET_AUDIO_MODE, mode, 0, 0, 0, NULL);
	return(true);
}

bool AndroidAccessory::hidRegist(uint8_t id, unsigned short descsize)
{
	usb.ctrlReq(1, 0,
                USB_SETUP_HOST_TO_DEVICE |
                USB_SETUP_TYPE_VENDOR |
                USB_SETUP_RECIPIENT_DEVICE,
                ACCESSORY_REGISTER_HID, id, 0, descsize, 0, NULL);
	return(true);
}

bool AndroidAccessory::hidUnRegist(uint8_t id)
{
	usb.ctrlReq(1, 0,
                USB_SETUP_HOST_TO_DEVICE |
                USB_SETUP_TYPE_VENDOR |
                USB_SETUP_RECIPIENT_DEVICE,
                ACCESSORY_UNREGISTER_HID, id, 0, 0, 0, NULL);
	return(true);
}

bool AndroidAccessory::hidSetReportDesc(uint8_t id, char* desc, int len)
{
    unsigned short pktSize = (unsigned short)epRecord[0].MaxPktSize; 
    unsigned short index = 0;
    unsigned short remain = (unsigned short)len;
    unsigned short datalen;

    while(remain > 0){
        if( pktSize < remain ) {
          datalen = pktSize;
        }else{
          datalen = remain;
        }
        usb.ctrlReq(1, 0,
            USB_SETUP_HOST_TO_DEVICE |
            USB_SETUP_TYPE_VENDOR |
            USB_SETUP_RECIPIENT_DEVICE,
            ACCESSORY_SET_HID_REPORT_DESC, id, 0, index, datalen, desc);
        remain -= datalen;
        desc += datalen;
        index++;
    }
    return(true);
}

bool AndroidAccessory::hidSendEvent(uint8_t id, char* event, int len)
{
	usb.ctrlReq(1, 0,
        USB_SETUP_HOST_TO_DEVICE |
        USB_SETUP_TYPE_VENDOR |
        USB_SETUP_RECIPIENT_DEVICE,
        ACCESSORY_SEND_HID_EVENT, id, 0, 0, (unsigned short)len, event);
	return(true);
}

bool AndroidAccessory::isConnectedHID(uint8_t id, char *desc, int len)
{
    USB_DEVICE_DESCRIPTOR *devDesc = (USB_DEVICE_DESCRIPTOR *) descBuff;
    byte err;

    max.Task();
    usb.Task();

    if (!connected &&
        usb.getUsbTaskState() >= USB_STATE_CONFIGURING &&
        usb.getUsbTaskState() != USB_STATE_RUNNING) {
//        Serial.print("\nDevice addressed... ");
//        Serial.print("Requesting device descriptor.\n");

        err = usb.getDevDescr(1, 0, 0x12, (char *) devDesc);
        if (err) {
//            Serial.print("\nDevice descriptor cannot be retrieved. Trying again\n");
            return false;
        }
        protocol = getProtocol(1);
        if (protocol >= 2) {
//          Serial.print("device supports protocol 2 or higher\n");
        } else {
//          Serial.print("could not read device protocol version\n");
          return false;
        }

        epRecord[0] = *(usb.getDevTableEntry(0,0));
        usb.setDevTableEntry(1, epRecord);
//        usb.setConf( 1, 0, 1 ); //may be unwanted
        hidRegist(id, len);
        hidSetReportDesc(id, desc, len);
        usb.setUsbTaskState( USB_STATE_RUNNING );
        connected = true;
        firstTime = true;
    } else if (usb.getUsbTaskState() == USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE) {
        if (connected)
            Serial.println("disconnect\n");
        connected = false;
        firstTime = false;
    }

    return connected;
}

void AndroidAccessory::clearFirstTime()
{
  firstTime = false;
}

