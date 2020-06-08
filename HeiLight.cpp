/*
Copyright(c) 2020 Johannes Schulte-Wuelwer

Distributed under GPL-3.0 license
*/

#include "HeiLight.h"
#include <iostream>
#include <unistd.h>

/*
        2 Bytes - RemoteID
        1 Byte  - Brightness
        1 Byte  - Color
        1 Byte  - Key
        1 Byte  - Sequence No
        1 byte  - 04 ???
        
        Color:
        gegen den Uhrzeigersinn
        00 - 9 Uhr


        Brightness:  160 - 179
        A0 - Low
        B0 - High
        
        KEY:
        01 - Alles an
        02 - Alles aus
        06 - Gruppe 1 an
        07 - Gruppe 1 aus
        08 - Gruppe 2 an
        09 - Gruppe 2 aus
        0A - gruppe 3 an
        0B - Gruppe 3 aus
        0C - Gruppe 4 an
        0D - Gruppe 4 aus
        0F - Brightness
        0E - Color
        10 - RGB
        8x - Longpress (geht nicht für RGB)
*/

HeiLight::HeiLight(AbstractPL1167 *pl1167) 
: mSequenceNumber(1), SmartLight(pl1167) {
}

void HeiLight::switchON(uint16_t remoteID, uint8_t groupID) {
    uint8_t remoteID0 = (remoteID >> 8) & 0xFF;
    uint8_t remoteID1 = (remoteID) & 0xFF;
    uint8_t _groupID;
    if (groupID == 0)
        _groupID = 1;
    else
        _groupID = 4 + (groupID * 2);
    uint8_t packet[] = {remoteID0, remoteID1, 0xAF, 0x00, _groupID, mSequenceNumber++, 0x04};

    setSyncwords(mSyncwords[0], mSyncwords[1]);
    write(mChannels, packet, sizeof(packet), 9);
}

void HeiLight::switchOFF(uint16_t remoteID, uint8_t groupID) {
    uint8_t remoteID0 = (remoteID >> 8) & 0xFF;
    uint8_t remoteID1 = (remoteID) & 0xFF;
    uint8_t _groupID;
    if (groupID == 0)
        _groupID = 2;
    else
        _groupID = 5 + (groupID * 2);
    uint8_t packet[] = {remoteID0, remoteID1, 0xAF, 0x00, _groupID, mSequenceNumber++, 0x04};
    
    setSyncwords(mSyncwords[0], mSyncwords[1]);
    write(mChannels, packet, sizeof(packet), 5);
}

void HeiLight::setBrightness(uint16_t remoteID, uint8_t groupID, int brightness) {
    uint8_t remoteID0 = (remoteID >> 8) & 0xFF;
    uint8_t remoteID1 = (remoteID) & 0xFF;
    
    // Valid range for brightness is 1 to 20
    brightness = brightness / 5;
    if (brightness < 1) brightness = 1;
    if (brightness > 20) brightness = 20;
    brightness = brightness + 159;
    uint8_t packet[] = {remoteID0, remoteID1, (uint8_t)brightness, 0x00, 0x0F, mSequenceNumber++, 0x04};

    setSyncwords(mSyncwords[0], mSyncwords[1]);
    write(mChannels, packet, sizeof(packet), 5);
}

void HeiLight::setColor(uint16_t remoteID, uint8_t groupID, int hue) {
    uint8_t remoteID0 = (remoteID >> 8) & 0xFF;
    uint8_t remoteID1 = (remoteID) & 0xFF;
    hue = hue * 255 / 359;
    uint8_t packet[] = {remoteID0, remoteID1, 0xA0, (uint8_t)hue, 0x0E, mSequenceNumber++, 0x04};

    setSyncwords(mSyncwords[0], mSyncwords[1]);
    setBrightness(remoteID, groupID, 0);
    usleep(500000);
    write(mChannels, packet, sizeof(packet), 5);
}

void HeiLight::setColorTemperature(uint16_t remoteID, uint8_t groupID, int colorTemp) {
    // Currently only support 'white' which is 4000 Kelvin
    // IoBroker IoT Adapter sends 250 for white
    if (colorTemp == 250) {
        setBrightness(remoteID, groupID, 100);
    }
}
