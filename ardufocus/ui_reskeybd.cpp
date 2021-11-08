/**
 * Ardufocus - Moonlite compatible focuser
 * Copyright (C) 2017-2019 João Brázio [joao@brazio.org]
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

#include "ui_reskeybd.h"

#ifdef USE_UI_KAP_ADV

// // Needed for debugging
// #include <stdio.h>
// #include <string.h>
// // End needed for debugging

#include "config.h"
#include "analog.h"
#include "api.h"
#include "io.h"
#include "moonlite.h"

const unsigned int ResKeybd::button_values[] = {
// Measured values: 
// 957, 729, 653, 177, 552, // B1-B5, with-out shift
// 837, 450, 366, 62, 277   // B1-B5, with shift pressed

      // Start of interval, End of interval, Action, <repeat>
      900, 990, NOTHING,      // B1
      800, 899, MOTOR_SWITCH, // Bs+B1
      690, 799, FAST_FWD,     // B2
      400, 500, ULTRA_FWD,    // Bs+B2
      111, 210, FAST_BWD,     // B4
       30, 110, ULTRA_BWD,    // Bs+B4
      600, 689, SLOWEST_FWD,  // B3
      330, 399, SLOW_FWD,     // Bs+B3
      501, 600, SLOWEST_BWD,  // B5
      211, 350, SLOW_BWD,     // Bs+B5
    };

motor_t ResKeybd::motor = MOTOR_ONE;
bool ResKeybd::inited = false;


void ResKeybd::setup()
{
    // Only run through this once.
    if(inited) { return; }
    inited = true;

  #ifdef UI_KAP_FWD_BUTTON_LED_PIN
    IO::set_as_output(UI_KAP_FWD_BUTTON_LED_PIN);
    IO::write(UI_KAP_FWD_BUTTON_LED_PIN, LOW);
  #endif

  #ifdef UI_KAP_BWD_BUTTON_LED_PIN
    IO::set_as_output(UI_KAP_BWD_BUTTON_LED_PIN);
    IO::write(UI_KAP_BWD_BUTTON_LED_PIN, LOW);
  #endif

  #ifdef UI_KAP_MOTOR_BUTTON_LED_PIN
    IO::set_as_output(UI_KAP_MOTOR_BUTTON_LED_PIN);
    IO::write(UI_KAP_MOTOR_BUTTON_LED_PIN, LOW);
  #endif
}

ResKeybd::action ResKeybd::decode(uint16_t value) {
    for (unsigned int i = 0; i < sizeof(button_values)/sizeof(unsigned int); i+=3) {
        if (button_values[i] <= value && value <= button_values[i+1]) {
            return static_cast<ResKeybd::action>(button_values[i+2]);
        }
    }
    return action::NOTHING;
} 

void ResKeybd::tick()
{
    setup();

    // Routine uses *_state variables for current state of switches, that then undergo a debouncing.
    // *_trigger then signal, that a change happened (once and only once after debouncing).  

    // Init to "off"
    bool fwd_state = false;
    bool bwd_state = false;
    bool switch_state = false;

    uint32_t new_motor_speed = 0;

    // Check if a button in the resistance network is pressed 
    uint16_t value = Analog::read(UI_KAP_ADC_CHANNEL);

    // // DEBUG See which values are returned
    // if (value > 10) {
    //     char buffer[30] = {0};
    //     sprintf_P(buffer, PSTR("%d"), value);
    //     comms.reply(buffer);
    // }

    enum action act = decode(value);
    switch (act) {
        case NOTHING:
            break;
        case SLOWEST_FWD:
            new_motor_speed = (motor == MOTOR_ONE) ? MOTOR1_MIN_SPEED : MOTOR2_MIN_SPEED;
            fwd_state = true;
            break;
        case SLOW_FWD:
            new_motor_speed = (motor == MOTOR_ONE) ? MOTOR1_MIN_SPEED + (MOTOR1_MAX_SPEED - MOTOR1_MIN_SPEED) / 3 : 
                                                     MOTOR2_MIN_SPEED + (MOTOR2_MAX_SPEED - MOTOR2_MIN_SPEED) / 3 ;
            fwd_state = true;
            break;
        case FAST_FWD:
            new_motor_speed = (motor == MOTOR_ONE) ? MOTOR1_MIN_SPEED + (MOTOR1_MAX_SPEED - MOTOR1_MIN_SPEED) * 2 / 3 : 
                                                     MOTOR2_MIN_SPEED + (MOTOR2_MAX_SPEED - MOTOR2_MIN_SPEED) * 2 / 3 ;
            fwd_state = true;
            break;
        case ULTRA_FWD:
            new_motor_speed = (motor == MOTOR_ONE) ? MOTOR1_MAX_SPEED : MOTOR2_MAX_SPEED;
            fwd_state = true;
            break;
        case SLOWEST_BWD:
            new_motor_speed = (motor == MOTOR_ONE) ? MOTOR1_MIN_SPEED : MOTOR2_MIN_SPEED;
            bwd_state = true;
            break;
        case SLOW_BWD:
            new_motor_speed = (motor == MOTOR_ONE) ? MOTOR1_MIN_SPEED + (MOTOR1_MAX_SPEED - MOTOR1_MIN_SPEED) / 3 : 
                                                     MOTOR2_MIN_SPEED + (MOTOR2_MAX_SPEED - MOTOR2_MIN_SPEED) / 3 ;
            bwd_state = true;
            break;
        case FAST_BWD:
            new_motor_speed = (motor == MOTOR_ONE) ? MOTOR1_MIN_SPEED + (MOTOR1_MAX_SPEED - MOTOR1_MIN_SPEED) * 2u / 3 : 
                                                     MOTOR2_MIN_SPEED + (MOTOR2_MAX_SPEED - MOTOR2_MIN_SPEED) * 2u / 3 ;
            bwd_state = true;
            break;
        case ULTRA_BWD:
            new_motor_speed = (motor == MOTOR_ONE) ? MOTOR1_MAX_SPEED : MOTOR2_MAX_SPEED;
            bwd_state = true;
            break;
        case MOTOR_SWITCH: 
            switch_state = true;
            break;
        default:
            break;
    }

    // Debounce routine, resets *_state back to false, if not pressed down long enough
    static uint8_t counter[3] = { 0 };
    static bool previous_state[3] = { false };

    bool fwd_trigger = false;
    debounce(fwd_state, previous_state[0], fwd_trigger, counter[0], UI_KAP_BUTTON_DEBOUNCE);

    bool bwd_trigger = false;
    debounce(bwd_state, previous_state[1], bwd_trigger, counter[1], UI_KAP_BUTTON_DEBOUNCE);

    bool switch_trigger = false;
    debounce(switch_state, previous_state[2], switch_trigger, counter[2], UI_KAP_BUTTON_DEBOUNCE);

    // Visual feedback when one of the forward buttons is pressed
    #ifdef UI_KAP_FWD_BUTTON_LED_PIN
    IO::write(UI_KAP_FWD_BUTTON_LED_PIN, (fwd_state) ? HIGH : LOW);
    #endif

    // Visual feedback when one of the backward buttons is pressed
    #ifdef UI_KAP_BWD_BUTTON_LED_PIN
    IO::write(UI_KAP_BWD_BUTTON_LED_PIN, (bwd_state) ? HIGH : LOW);
    #endif

    // // Visual feedback when the Motor button is pressed.
    // #ifdef UI_KAP_MOTOR_BUTTON_LED_PIN
    // IO::write(UI_KAP_MOTOR_BUTTON_LED_PIN, (switch_state) ? HIGH : LOW);
    // #endif

    static uint32_t old_motor_speed = 0;

    if(fwd_state || bwd_state) {
        if(fwd_trigger || bwd_trigger) {
            old_motor_speed = api::motor_get_speed(motor);
        }

        // It's important that the "new_motor_speed" value not to
        // be lower than 2, because at the stepper tick routine it
        // will be divivded by two.
        // api::motor_set_speed(motor, new_motor_speed);
        api::motor_set_speed(motor, map(new_motor_speed, MOTOR1_MIN_SPEED, MOTOR1_MAX_SPEED, 2, 64));


        if(! api::motor_is_moving(motor)) {
            api::motor_set_target(motor, (fwd_state) ? -1 : 0); // Max of movement range (overflows as unsigned), Start of movement range. 
            api::motor_start(motor);
            // comms.reply("s");
        }
    } else {
        // Buttons not pressed
        if(fwd_trigger || bwd_trigger) {
            api::motor_stop(motor);
            api::motor_set_speed(motor, old_motor_speed);
            // comms.reply("x");
        }

        #ifdef MOTOR2_HAS_DRIVER

        #ifdef UI_KAP_MOTOR_BUTTON_LED_PIN
        IO::write(UI_KAP_MOTOR_BUTTON_LED_PIN, (motor == MOTOR_ONE) ? HIGH : LOW);
        #endif

        if(switch_state && switch_trigger && ! api::motor_is_moving(motor)) {
            switch(motor) {
                case MOTOR_ONE:
                comms.reply("1");
                motor = MOTOR_TWO;
                break;

                case MOTOR_TWO:
                comms.reply("2");
                motor = MOTOR_ONE;
                break;
            }
        }
        #endif // MOTOR2_HAS_DRIVER
    }
}

void ResKeybd::debounce(bool& current_state, bool& previous_state, bool& trigger_event, uint8_t& counter, const uint8_t& threshold)
{
  setup();

  if(current_state == previous_state) {
    trigger_event = false;
    counter = 0;
    return;
  }

  if(counter < threshold) {
    current_state = previous_state;
    trigger_event = false;
    ++counter;
    return;
  }

  previous_state = current_state;
  trigger_event = true;
  counter = 0;
  return;
}

#endif 
