/*
Copyright(c) 2020 Johannes Schulte-Wuelwer

Distributed under GPL-3.0 license
*/

#ifndef SMARTLIGHT_H_
#define SMARTLIGHT_H_

#include <vector>
#include "external/openmilight/AbstractPL1167.h"

class SmartLight {
    private:
        static uint8_t mOutPacket[8];
        static AbstractPL1167 *mPL1167;
        static uint16_t mSW[2];

    protected:
        const int mBulbType = 0;

        void setSyncwords(uint16_t syncwordA, uint16_t syncwordB);
        int write(std::vector<uint8_t> channels, uint8_t frame[], size_t frame_length, int resends = 1);

    public:
        SmartLight(AbstractPL1167 *pl1167);
        virtual void switchON(uint16_t remoteID, uint8_t groupID) = 0;
        virtual void switchOFF(uint16_t remoteID, uint8_t groupID) = 0;
        virtual void setBrightness(uint16_t remoteID, uint8_t groupID, int brightness) = 0;
        virtual void setColor(uint16_t remoteID, uint8_t groupID, int hue) = 0;
        virtual void setColorTemperature(uint16_t remoteID, uint8_t groupID, int colorTemp) = 0;
};

#endif /* SMARTLIGHT_H_ */
