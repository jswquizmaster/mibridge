/*
Copyright(c) 2020 Johannes Schulte-Wuelwer

Distributed under GPL-3.0 license
*/

#include "MiLight.h"
#include <iostream>
#include <unistd.h>

/*
        1 Byte  - 0xB0 or 0xB8 Prefix ???
        2 Bytes - RemoteID
        1 Byte  - Color
        1 Byte  - Brightness
        1 Byte  - Key
        1 Byte  - Sequence No
*/

MiLight::MiLight(AbstractPL1167 *pl1167) 
: mSequenceNumber(1), mHue(0), mBrightness(0xA8), SmartLight(pl1167) {
}

void MiLight::switchON(uint16_t remoteID, uint8_t groupID) {
    uint8_t remoteID0 = (remoteID >> 8) & 0xFF;
    uint8_t remoteID1 = (remoteID) & 0xFF;
    uint8_t _groupID = 0x11 + (groupID * 2);
    
    uint8_t packet[] = {0xB0, remoteID0, remoteID1, 0x00, 0x00, _groupID, mSequenceNumber++};
    setSyncwords(mSyncwords[0], mSyncwords[1]);
    write(mChannels, packet, sizeof(packet), 9);
}

void MiLight::switchOFF(uint16_t remoteID, uint8_t groupID) {
    uint8_t remoteID0 = (remoteID >> 8) & 0xFF;
    uint8_t remoteID1 = (remoteID) & 0xFF;
    uint8_t _groupID = 2 + (groupID * 2);
    uint8_t packet[] = {0xB0, remoteID0, remoteID1, 0x00, 0x00, _groupID, mSequenceNumber++};

    setSyncwords(mSyncwords[0], mSyncwords[1]);
    write(mChannels, packet, sizeof(packet), 9);
}

void MiLight::setBrightness(uint16_t remoteID, uint8_t groupID, int brightness) {
    uint8_t remoteID0 = (remoteID >> 8) & 0xFF;
    uint8_t remoteID1 = (remoteID) & 0xFF;
    uint8_t _groupID = 0x01 + (groupID * 2);    
    uint8_t packet[] = {0xB8, remoteID0, remoteID1, mHue, mBrightness, _groupID, mSequenceNumber++};

    setSyncwords(mSyncwords[0], mSyncwords[1]);
    write(mChannels, packet, sizeof(packet), 9);

    // Valid range for brightness is 0 to 100
    if (brightness < 0) brightness = 0;
    if (brightness > 100) brightness = 100;
    // Expect an input value in [0, 100]. Map it down to [0, 25].
    brightness = brightness * 25 / 100;
    // The actual protocol uses a bizarre range where min is 16, max is 23:
    // [16, 15, ..., 0, 31, ..., 23]
    mBrightness = (((31 - brightness) + 17) % 32)<<3;

    packet[4] = mBrightness;
    packet[5] = 0x0E;
    packet[6] = mSequenceNumber++;
    write(mChannels, packet, sizeof(packet), 9);
}

void MiLight::setColor(uint16_t remoteID, uint8_t groupID, int hue) {
    uint8_t remoteID0 = (remoteID >> 8) & 0xFF;
    uint8_t remoteID1 = (remoteID) & 0xFF;
    uint8_t _groupID = 0x01 + (groupID * 2);
    hue = (hue + 40) % 360;
    hue = hue * 255 / 359;
    
    setSyncwords(mSyncwords[0], mSyncwords[1]);

    uint8_t packet[] = {0xB8, remoteID0, remoteID1, (uint8_t)hue, 0xBB, _groupID, mSequenceNumber++};
    write(mChannels, packet, sizeof(packet), 9);
    packet[0] = 0xB0;
    packet[5] = 0x0F;
    packet[6] = mSequenceNumber++;
    write(mChannels, packet, sizeof(packet), 9);
}

void MiLight::setColorTemperature(uint16_t remoteID, uint8_t groupID, int colorTemp) {
    // Currently only support 'white' which is 4000 Kelvin
    if (colorTemp == 4000) {
        switchON(remoteID, groupID);
    }
}
