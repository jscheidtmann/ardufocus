/**
 * Ardufocus - Moonlite compatible focuser
 * Copyright (C) 2017-2019 João Brázio [joao@brazio.org]
 * Copyright (C) 2021 Jens Scheidtmann [jens.scheidtmann@gmail.com]
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __RESKEYBD_H__
#define __RESKEYBD_H__

#include "config.h"
#include "analog.h"
#include "api.h"
#include "io.h"

/**
 * @brief Class  "ResistornetKeyBoard" for managing a keyboard implemented using a resistance network
 * 
 * This class implements a keyboard consisting of a network of resistances. 
 * It is connected to one of the analog-in ports of the arduino (Ax, x=0..7).
 * In prinicple you can use as many resistances as you would like, but practically
 * the number of buttons is limited by the resolution of arduino's ADC (1023 steps).
 * 
 * You can use the following network, which features 5 buttons + shift:
 * 
 * \parblock 
 * VREF ------*--------*--------*--------*--------*
 *            |        |        |        |        |
 *           ---      ---      ---      ---      ---
 *           | |      | |      | |      | |      | |
 *           ---R1    ---R2    ---R3    ---R4    ---R5
 *            |        |        |        |        |
 *            *        *        *        *        *
 *             \        \        \        \        \
 *            * B1     * B2     * B3     * B4     * B5
 *            |        |        |        |        |
 * Ax         *--------*---*----*----*---*--------*
 *                         |         |
 *                        ---       ---
 *                        | |       | |
 *                        ---R6     ---Rs
 *                         |         |
 *                         |         *
 *                         |          \
 *                         |         * Bs
 *                         |         |
 * GND --------------------*---------* 
 * @endparblock
 * 
 * Note that in this resistor network, Bs serves as "shift" button and pressing it, will not result in an action.
 * 
 * The default assignment is:
 * 
 * ------------------------------------------------------------------
 * | Button | Action ID  | Action                                   |
 * ------------------------------------------------------------------
 * |   B1   |  STEP_FWD    | Move selected motor 1 step forward     |
 * |  Bs+B1 | SMALL_FWD    | Move selected motor 10 steps forward   |
 * |   B2   |  STEP_BWD    | Move selected motor 1 step backward    |
 * |  Bs+B2 | SMALL_BWD    | Move selected motor 10 steps backward  |
 * |   B3   | MEDIUM_FWD   | Move selected motor 50 steps forward   |
 * |  Bs+B3 | LARGE_FWD    | Move selected motor 100 steps forward  |
 * |   B4   | MEDIUM_BWD   | Move selected motor 50 steps backward  |
 * |  Bs+B4 | LARGE_BWD    | Move selected motor 100 steps backward |
 * ------------------------------------------------------------------
 * |   B5   |    -/-       | <not assigned>                         |
 * |  Bs+B5 | MOTOR_SWITCH | Switch from Motor 1 to motor 2 or back |
 * ------------------------------------------------------------------
 * 
 * This assignemnt of resistances yield the maximum sparation between signals:
 * 
 * --------------------------
 * | Resistance | Value/Ohm |
 * --------------------------
 * |     R1     |     820   | 
 * |     R2     |     4.7k  |
 * |     R3     |     6.8k  |
 * |     R4     |     10k   |
 * |     R5     |     56k   |
 * |     R6     |     12k   |
 * |     Rs     |     5.6k  |
 * --------------------------
 * 
 * If you do not use these resistances, you'll need to edit the values in XXX below.
 */
class ResKeybd
{
  /**
   * Disable the creation of an instance of this object.
   * This class should be used as a singleton.
   */
  private:
     ResKeybd() {;}
    ~ResKeybd() {;}

  private:
    static motor_t motor;
    static bool inited;

  public:
    /**
     * @brief Actions defined by the keyboard
     * 
     */
    enum action { 
        NOTHING = 0, 
        SLOWEST_FWD = 1, 
        SLOWEST_BWD = 2, 
        SLOW_FWD = 3, 
        SLOW_BWD = 4, 
        FAST_FWD = 5, 
        FAST_BWD = 6, 
        ULTRA_FWD = 7, 
        ULTRA_BWD = 8, 
        MOTOR_SWITCH = 9,
    };
    
    /**
     * @brief Button values as read from the ADC channel.
     * 
     * You can use actual values that you measured, or calculated values.
     * Also @see delta below.
     * 
     * An entry always consists of 3 values, start of interval, end of intervall and an action. 
     * 
     * Note: First matching action will be taken. 
     */
    static const unsigned int button_values[30]; 

    /**
     * @brief Setup the class
     * 
     * Configures the PINs of the Arduino
     * 
     */
    static void setup();

    /**
     * @brief Method called in the main loop
     * 
     */
    static void tick();

    /**
     * @brief Helper method for debouncing button presses.
     * 
     * @param current_state, status of "button" as read from ADC. 
     * @param previous_state, status of "button" as previously defined.
     */
    static void debounce(bool& current_state, bool& previous_state, bool& trigger_event, uint8_t& counter, const uint8_t& threshold);

    /**
     * @brief Helper method for determining which action to take.
     * 
     * @param value, the value read from the ADC.
     * @return enum action, action as decoded
     */
    static enum action decode(uint16_t value);
};

#endif
