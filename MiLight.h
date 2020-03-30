/*
Copyright(c) 2020 Johannes Schulte-Wuelwer

Distributed under GPL-3.0 license
*/

#ifndef MILIGHT_H_
#define MILIGHT_H_

#include "SmartLight.h"

class MiLight: public SmartLight  {
    protected:
        const uint16_t mSyncwords[2] = {0x147A, 0x258B};
        const std::vector<uint8_t> mChannels = {9, 40, 71};
        const int mBulbType = 0;
        uint8_t mSequenceNumber;
        uint8_t mHue;
        uint8_t mBrightness;

    public:
        MiLight(AbstractPL1167 *pl1167);
        void switchON(uint16_t remoteID, uint8_t groupID);
        void switchOFF(uint16_t remoteID, uint8_t groupID);
        void setBrightness(uint16_t remoteID, uint8_t groupID, int brightness);
        void setColor(uint16_t remoteID, uint8_t groupID, int hue);
        void setColorTemperature(uint16_t remoteID, uint8_t groupID, int colorTemp);
};

#endif /* MILIGHT_H_ */
