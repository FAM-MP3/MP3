/*
 * LabADC.hpp
 *
 *  Created on: Oct 1, 2018
 *      Author: kimfl
 */

#ifndef LABADC_HPP_
#define LABADC_HPP_

#include <stdio.h>
#include "io.hpp"

class LabADC
{
public:
    enum Pin
    {
        k0_25,       // AD0.2 <-- Light Sensor -->
        k0_26,       // AD0.3
        k1_30,       // AD0.4
        k1_31,       // AD0.5

        /* These ADC channels are compromised on the SJ-One,
         * hence you do not need to support them
         */
        // k0_23 = 0,   // AD0.0
        // k0_24,       // AD0.1
        // k0_3,        // AD0.6
        // k0_2         // AD0.7
    };

    // Nothing needs to be done within the default constructor
    LabADC();

    /**
    * 1) Powers up ADC peripheral
    * 2) Set peripheral clock
    * 2) Enable ADC
    * 3) Select ADC channels
    * 4) Enable burst mode
    */
    void AdcInitBurstMode();

    /**
    * 1) Selects ADC functionality of any of the ADC pins that are ADC capable
    *
    * @param pin is the LabAdc::Pin enumeration of the desired pin.
    *
    * WARNING: For proper operation of the SJOne board, do NOT configure any pins
    *           as ADC except for 0.26, 1.31, 1.30
    */
    void AdcSelectPin(Pin pin);

    /**
    * 1) Returns the voltage reading of the 12bit register of a given ADC channel
    * You have to convert the ADC raw value to the voltage value
    * @param channel is the number (0 through 7) of the desired ADC channel.
    */
    float ReadAdcVoltageByChannel(uint8_t channel);
};



#endif /* LABADC_HPP_ */
