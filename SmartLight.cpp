/*
Copyright(c) 2020 Johannes Schulte-Wuelwer

Distributed under GPL-3.0 license
*/

#include <cstring>
#include "SmartLight.h"

#include <stdio.h>

AbstractPL1167 *SmartLight::mPL1167 = 0;
uint8_t SmartLight::mOutPacket[8];

SmartLight::SmartLight(AbstractPL1167 *pl1167) {
    if (mPL1167 == 0) {
        mPL1167 = pl1167;
        mPL1167->open();
        mPL1167->setCRC(true);
        mPL1167->setPreambleLength(3);
        mPL1167->setTrailerLength(4);
        mPL1167->setSyncword(mSyncwords[0], mSyncwords[1]);
        mPL1167->setMaxPacketLength(8);
    }    
}

void SmartLight::setSyncwords(uint16_t syncwordA, uint16_t syncwordB) {
    if((mSyncwords[0] == syncwordA) && (mSyncwords[1] == syncwordB))
        return;

    mSyncwords[0] = syncwordA;
    mSyncwords[1] = syncwordB;
    mPL1167->setSyncword(syncwordA, syncwordB);
}

int SmartLight::write(std::vector<uint8_t> channels, uint8_t frame[], size_t frame_length, int resends)
{
    if (frame_length > sizeof(mOutPacket) - 1) {
        return -1;
    }

    memcpy(mOutPacket + 1, frame, frame_length);
    mOutPacket[0] = frame_length;

    /*
    printf("2.4GHz --> Sending: ");
    for (int i = 0; i < 8; i++) {
      printf("%02X ", mOutPacket[i]);
    }
    printf(" [x%d]\n", resends);
    */
    
    for (int i = 0; i < resends; i++) {
        for (size_t channel = 0; channel < channels.size(); channel++) {
            //printf("CHANNEL: %d\n", channels[channel]);
            mPL1167->writeFIFO(mOutPacket, mOutPacket[0] + 1);
            mPL1167->transmit(channels[channel]);
        }
    }

    return frame_length;
}
