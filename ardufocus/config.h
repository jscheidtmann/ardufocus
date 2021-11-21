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

#ifndef __CONFIG_H__
#define __CONFIG_H__

// ----------------------------------------------------------------------------
// FULL CONFIGURATION ---------------------------------------------------------
// ----------------------------------------------------------------------------
//
// The following config.h shows a full configuration for ardufocus, for using
// two motors, 6 buttons (FWD, BCK, Fast, FWD, Fast BCK, Switch between motors), 
// 3 LEDs (FWD, BCK, Motor), 1 NTC
//
// The number of pins that are usable is limited to at max 19 pins, according
// to the hardware abstraction layer (hal.h).

// 19 - 12 (2x A4988) = 7 - 2 (Serial) = 5!
// TODO

// ----------------------------------------------------------------------------
// PERSISTENT MEMORY ----------------------------------------------------------
// ----------------------------------------------------------------------------
//
// Enable auto save of the focuser position uppon finishing each move. When
// active the focuser will remember between power cycles the exact position
// where it was.
#define USE_EEPROM


// ----------------------------------------------------------------------------
// MISCELLANEOUS --------------------------------------------------------------
// ----------------------------------------------------------------------------
//
// Remote reset is a non standard feature added to the Moonlite protocol which
// allows you to reset the micro controller inside the focuser. Please test
// this feature on your bench before deploying on the field. For this feature
// to work the default boot loader may need to be changed otherwise the uC will
// enter into an infinite loop state.
//
// For more information read the [bug report].
//
// [bug report]: https://github.com/arduino/Arduino/issues/4492
//#define ENABLE_REMOTE_RESET

// Enable a subset of commands to control the status of the DTR auto-reset
// feature on the Arduino boards. By default Ardufocus uses a cap between the
// reset pin and ground to prevent the DTR signal to reset the board, which
// happens everytime a new serial connection is open to the board.
// By having the cap between the reset pin and another board pin, enabling
// this feature allows the DTR auto-reset to be controlled. This is usefull
// for firmware uploads, if the DTR auto-reset is disabled the user must
// manually press the reset button when uploading new firmware.
//#define ENABLE_DTR_RESET

// This is the board pin which controls the enable/disable of the feature.
//#define DTR_RESET_PINOUT 15

// Activating this option will enable high resolution counters (32-bit) thus
// becoming incompatible with the standard Moonlite protocol. You should enable
// this if using a gearbox or having a milimetric threaded rod on the drive
// mechanism.
//#define HIGH_RESOLUTION_MODE

// ----------------------------------------------------------------------------
// MOTOR #1 CONFIGURATION -----------------------------------------------------
// ----------------------------------------------------------------------------
//
// You should only enable *ONE* of the following drivers
// The ULN2003 shall be used with the unmodded version of 28BYJ-48 or any other
// Unipolar stepper motor. The A4988 driver should be used with Bipolar stepper
// motors or the modded version of the 28BYJ-48 (see the doc/ folder).
#define MOTOR1_USE_A4988_DRIVER
//#define MOTOR1_USE_DRV8825_DRIVER
//#define MOTOR1_USE_ULN2003_DRIVER

// Driver pin-out definition
// Define bellow the pin-out for your specific driver.
#ifdef MOTOR1_USE_ULN2003_DRIVER
  //                    IN1, IN2, IN3, IN4
  #define MOTOR1_PINOUT   2,   3,   4,   5
#endif

#ifdef MOTOR1_USE_A4988_DRIVER
  //                    MS1, MS2, MS3, SLEEP,  STEP, DIR
  #define MOTOR1_PINOUT  12,  11,  10,     8,     7,   6
#endif

// Activate the following directive if you'd like to invert the motor rotation
// changing the focus direction.
//#define MOTOR1_INVERT_DIRECTION

// When active Ardufocus will cut the stepper motor current when idle, in theory
// this could lead to less accuracy between movements but will keep the motor
// cool. When disabling this flag make sure your motor does not overheat.
#define MOTOR1_SLEEP_WHEN_IDLE

// When the previous directive is active, allows you to control the amount of
// time the driver will wait, after stopping, before powering down the motor.
// The idea behind is the system needs full accuracy between the AF point
// sequence but can be powered down between AF runs. Thus you should set this
// to wait a bit more than the time your system needs to take and measure each
// AF point.
#define MOTOR1_SLEEP_TIMEOUT 15

// ----------------------------------------------------------------------------
// MOTOR #2 CONFIGURATION -----------------------------------------------------
// ----------------------------------------------------------------------------
// See above for explanations 

//// Activate one of the following lines to activate a second motor
#define MOTOR2_USE_A4988_DRIVER
//#define MOTOR2_USE_DRV8825_DRIVER
//#define MOTOR2_USE_ULN2003_DRIVER

// Driver pin-out definition
// Define bellow the pin-out for your specific driver.
#ifdef MOTOR2_USE_ULN2003_DRIVER
  //                    IN1, IN2, IN3, IN4
  #define MOTOR2_PINOUT   2,   3,   4,   5
#endif

#ifdef MOTOR2_USE_A4988_DRIVER
  //                    MS1, MS2, MS3, SLEEP,  STEP, DIR
  #define MOTOR2_PINOUT  18,  17,  16,    15,    14,  13
#endif

//#define MOTOR2_INVERT_DIRECTION

#define MOTOR2_SLEEP_WHEN_IDLE
#define MOTOR2_SLEEP_TIMEOUT 15

// ----------------------------------------------------------------------------
// SPEED PROFILE --------------------------------------------------------------
// ----------------------------------------------------------------------------
// Specify a custom speed profile for you motor model and driver combo.
// The units are in steps/sec.
//
// Usually a motor such as the 28BYJ-48 need lower speed limits:
//  - Max speed: 250
//  - Min speed: 25
//
// NEMA17 motors allow you to use higher speed limits:
//  - Max speed: 1000
//  - Min speed: 250
//
#define MOTOR1_MAX_SPEED 500
#define MOTOR1_MIN_SPEED 25

#define MOTOR2_MAX_SPEED 500
#define MOTOR2_MIN_SPEED 25

// Define steps size per motor (microstepping)
// Allowed values are 1 = full stepping, 2 = half step, 4 = quarter step
// (some drivers allow even smaller steps, see api.h for implementation)
#define MOTOR1_MICROSTEPPING 4
#define MOTOR2_MICROSTEPPING 4


// ----------------------------------------------------------------------------
// ACCELERATION PROFILE -------------------------------------------------------
// ----------------------------------------------------------------------------
// When active Ardufocus will apply the selected acceleration profile to the
// motor's speed. The objective is to help the system cope with heavier loads
// such as FF + FW + CCD combos.
//
//
// Linear Acceleration   Trapezoid Acceleration   Smooth Step Acceleration
//                                                        (S-Curve)
//
//   |   /\                |   ___________           |     __---__
// V |  /  \             V |  /           \        V |    -       -
//   | /    \              | /             \         |   -         -
//   |/      \             |/               \        |__-           -__
//   +----------------     +-------------------      +-------------------
//         T                        T                         T
//
//#define USE_LINEAR_ACCEL
#define USE_TRAPEZOID_ACCEL
//#define USE_SMOOTHSTEP_ACCEL

// The acceleration profile, independent of the method used, has at least two
// main periods: the ramp-up period when the motor is gaining speed and the
// ramp-down period when the motor is losing speed. This setting controls the
// duration of each one of those periods, the default value is 250 steps for
// each period if left undefined.
//#define ACCEL_DURATION 250

// When acceleration control is active this setting controls the minimum
// required number of steps on a movement for the algorithm to kick in. Any
// movement with less steps than this will be done at minimum speed without
// any acceleration control. The default value is 10 steps of left undefined.
//#define ACCEL_MIN_STEPS 10


// ----------------------------------------------------------------------------
// TEMPERATURE SENSOR ---------------------------------------------------------
// ----------------------------------------------------------------------------

// Comment out the following line to disable the temperature sensor 
// #define PROVIDE_NTC

// PIN to use
#define NTC_ADC_CHANNEL          4
#define NTC_NOMINAL_TEMP      25.0F
#define NTC_BCOEFFICIENT    3950.0F
#define NTC_NOMINAL_VAL    10000.0F
#define NTC_RESISTOR_VAL   10000.0F

#define NTC_MIN_RAW_VALUE  50 // min value to be considered a valid reading (xxxC)
#define NTC_MAX_RAW_VALUE 950 // max value to be considered a valid reading (-23C)

// According to the Moonlite protocol the temperature probe should only be read
// when the command ":C#" is received but some applications, such as SGP, seems
// not to respect this and only call the get temperature command ":GT#" which
// means the temperature will never get updated and the last read value is always
// returned, either it is valid or not. Enabling the following option will force
// the temperature gathering process on every temperature read command.
#define START_TEMP_CONVERSION_ON_EVERY_GET


// ----------------------------------------------------------------------------
// USER INTERFACE -------------------------------------------------------------
// ----------------------------------------------------------------------------

////////////////////
//   B A S I C   ///
////////////////////

// This is the most basic user interface, it uses two keys to move the focuser
// IN (FWD) and out (BWD). A third key (SWT) can be used to select the active
// motor on a dual motor configuration.
//#define USE_UI_KAP

#ifdef USE_UI_KAP
// Use the following settings to select the input pins connected to each one of
// the switches. The third button is optional (just comment it out).
//
// You need to add a switch to each of these pins, see comment below on active low
#define UI_KAP_FWD_BUTTON_PIN 9
#define UI_KAP_BWD_BUTTON_PIN 16
#define UI_KAP_SWT_BUTTON_PIN 5

// We like the switches to be wired in an active-low configuration, this way you
// don't need to use additional external resistors, we will automatically enable
// the internal pull-ups for you. 
//
//           ---/.---
//  GND --- | SWITCH | --- INPUT_PIN
//           --------
//
// If you decide to use any other wiring logic comment out the following line
#define UI_KAP_INVERT_BUTTON_LOGIC

#endif 

//////////////////////////
//   A D V A N C E D   ///
//////////////////////////

// The advanced UI capability uses a resistor network to provide 5 buttons + shift. 
// See ui_reskeybd.h for an example resistor network, that can be used.
#define USE_UI_KAP_ADV

#ifdef USE_UI_KAP_ADV

// Pin A0 is channel 0 (..) pin A3 is channel 3
// DO NOT USE CHANNEL 0, valid options are 1-5
#define UI_KAP_ADC_CHANNEL 5

#endif 

///////////////////////////////////
////   U I   F e e d b a c k   ////
///////////////////////////////////

// The options below allows you to have one LED per button, the LED will light
// up when the button is pressed. The third button (SWT) will be lit when the
// first motor is active, and off when the second motor is active.
//
// Note: These MUST be defined when using one of the UI capabilities.

#if defined(USE_UI_KAP) || defined(USE_UI_KAP_ADV) 

#define UI_KAP_FWD_BUTTON_LED_PIN 4
#define UI_KAP_BWD_BUTTON_LED_PIN 3
#define UI_KAP_MOTOR_BUTTON_LED_PIN 2

#endif


// ----------------------------------------------------------------------------
// DO NOT EDIT ANYTHING BELLOW THIS HEADER ------------------------------------
// ----------------------------------------------------------------------------
#include "assert.h"

#endif
