/*
Copyright(c) 2020 Johannes Schulte-Wuelwer

Distributed under GPL-3.0 license
*/

#ifndef HEILIGHT_H_
#define HEILIGHT_H_

#include "SmartLight.h"

class HeiLight: public SmartLight  {
    protected:
        const uint16_t mSyncwords[2] = {0x050A, 0x55AA};
        const std::vector<uint8_t> mChannels = {0x04, 0x4A};
        const int mBulbType = 0;
        uint8_t mSequenceNumber;

    public:
        HeiLight(AbstractPL1167 *pl1167);
        void switchON(uint16_t remoteID, uint8_t groupID);
        void switchOFF(uint16_t remoteID, uint8_t groupID);
        void setBrightness(uint16_t remoteID, uint8_t groupID, int brightness);
        void setColor(uint16_t remoteID, uint8_t groupID, int hue);
        void setColorTemperature(uint16_t remoteID, uint8_t groupID, int colorTemp);
};

#endif /* HEILIGHT_H_ */