/*
 * Android ADK AOA2.0 HID sample implementation
 * Copyright (C) 2012 Yuuichi Akagawa
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

#define JOYSTICK_VERT A0
#define JOYSTICK_HORZ A1
#define JOYSTICK_SEL  A2
#define DIVIDER 32

AndroidAccessory acc("ammlab.org",
                     "AOAHID",
                     "AOAHIDsample",
                     "1.0",
                     "https://github.com/YuuichiAkagawa/Arduino-AOA2",
                     "0000001");

const byte _hidReportDescriptor[] = {
//    Mouse
0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01,
0xA1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x08,
0x15, 0x00, 0x25, 0x01, 0x95, 0x08, 0x75, 0x01,
0x81, 0x02, 0x95, 0x00, 0x81, 0x03, 0x05, 0x01,
0x09, 0x30, 0x09, 0x31, 0x09, 0x38, 0x15, 0x81,
0x25, 0x7F, 0x75, 0x08, 0x95, 0x03, 0x81, 0x06,
0x05, 0x0C, 0x0A, 0x38, 0x02, 0x95, 0x01, 0x81,
0x06, 0xC0, 0xC0,
};

int vCenterH, vCenterL;
int hCenterH, hCenterL;

// Joystick center calibration
void calibration(){
    //Vertical
    int min = 1024;
    int max = 0;
    int val;
    for(int i=0; i<10; i++)
    {
        val = analogRead(A0);
        if(val < min){
            min = val;
        }
        if( val > max ){
            max = val;
        }
        delay(100);
    }
    vCenterH = max;
    vCenterL = min;

    //Horizontal
    min = 1024;
    max = 0;
    for(int i=0; i<10; i++)
    {
        val = analogRead(A1);
        if(val < min){
            min = val;
        }
        if( val > max ){
            max = val;
        }
        delay(100);
    }
    hCenterH = max;
    hCenterL = min;
}

char m[5];
void setup()
{
  Serial.begin(115200);
  pinMode(JOYSTICK_SEL, INPUT_PULLUP);
  m[0] = m[1] = m[2] = m[3] = m[4] = 0;
  
  calibration();
  acc.powerOn();
}

void loop()
{
  static uint8_t id = 0;
  if (acc.isConnectedHID(id, (char*)_hidReportDescriptor, sizeof(_hidReportDescriptor))) {

    //Read Joystick value
    int vVal = analogRead(JOYSTICK_VERT);
    int hVal = analogRead(JOYSTICK_HORZ);

     //Vertical
    if( vVal < vCenterH+20 && vVal > vCenterL-20 ){  //neutral
        m[2] = 0;
    }else{
        if(vVal > vCenterL){ //+
            m[2] = (byte)(((vVal - vCenterL)/DIVIDER) *-1);
        }else{
            m[2] = (byte)((vCenterL - vVal)/DIVIDER);
        }
    }
    
    //Horizontal
    if( hVal < hCenterH+20 && hVal > hCenterL-20 ){  //neutral
        m[1]= 0;
    }else{
        if(hVal > hCenterL){ //+
            m[1] = (byte)(((hVal - hCenterL)/DIVIDER) * -1);
        }else{
            m[1] = (byte)((hCenterL - hVal)/DIVIDER);
        }
    }
    
    //Push
    if( digitalRead(JOYSTICK_SEL) == 0 ){
        m[0] = 1;
    }else{
        m[0] = 0;
    }
    
    //Send HID event to Android
    acc.hidSendEvent(id, m, sizeof(m));
  }
  delay(10);

}
