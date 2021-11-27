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

#include "stepper.h"

/**
 * @brief Initialize the Stepper class
 * @details 
 * Initialize velocity
 *
 */
void stepper::init()
{
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    m_speed = 2;
    m_ovf_counter = 0;
  }
}


/**
 * @brief Start moving
 * @details 
 * Sets moving to true, so the motor starts moving. 
 *
 */
void stepper::move()
{
  m_ovf_counter = 0;
  m_position.moving = true;
}


/**
 * @brief Return boolean, telling if motor is moving.
 *
 */
bool stepper::is_moving()
{
  const bool b = (m_position.moving);
  return b;
}


/**
 * @brief halt motor
 * @details 
 * Stops motor movement, by setting moving to false.
 * Sets target position to current position.
 *
 */
void stepper::halt()
{
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    m_position.target = m_position.current;
    m_position.moving = false;
  }
}


/**
 * @brief Return current position
 * @details 
 * in a thread safe way, it seems.
 */
uint32_t stepper::get_current_position()
{
  const uint32_t c = m_position.current;
  return c;
}



/**
 * @brief Set the current position to the passed value
 * @details 
 * Additionally sets the target value to the current position.
 *
 */
void stepper::set_current_position(const uint32_t& target) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    m_position.current = target;
    m_position.target  = target;
  }
}


/**
 * @brief return the current speed
 * @details 
 * The larger this one is, the slower it is.
 */
uint16_t stepper::get_speed()
{
  const uint16_t s = m_speed;
  return s;
}


/**
 * @brief set current speed
 * @details 
 * Sets the speed divider. 
 *
 */
void stepper::set_speed(const uint16_t& target)
{
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    m_speed = target;
  }
}


/**
 * @brief [brief description]
 * @details [long description]
 *
 */
uint32_t stepper::get_target_position()
{
  const uint32_t t = m_position.target;
  return t;
}



/**
 * @brief set target the motor should move to
 * @details 
 * If one of the acceleration profiles is configured, members of m_position are calculated.
 * 
 * If only a small distance needs to be travelled, members are not calculated and min velocity is used (see tick()).
 * 
 * For Linear, the motor accelerates till reaching 1/2 distance, then decelerates.
 * For Trapezoid, an easein and easout relative position are calculated. Between these max velocity is used.
 * For Smooth, see tick() and the utility called there.
 * 
 * Calculations depend on 
 *  - ACCEL_MIN_STEPS, distance that needs to be travelled, to enable velocity profiles (both profiles)
 *  - ACCEL_DURATION, number of steps, over which the velocity increases/decreases. (Trapezoid only)
 *    If distance to travel is < 2*ACCEL_DURATION, Trapezoid is equivalent to Linear.
 * 
 * @see m_position
 */
void stepper::set_target_position(const uint32_t& target) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    m_position.target = target;

    #ifdef HAS_ACCELERATION
      m_position.relative = 0;

      m_position.distance = (m_position.current > target)
        ? m_position.current - target
        : target - m_position.current;

      if (m_position.distance >= ACCEL_MIN_STEPS)
      {
        #if defined(USE_LINEAR_ACCEL)
          m_position.easein  = m_position.distance >> 1;
          m_position.easeout = m_position.easein;

        #elif defined(USE_TRAPEZOID_ACCEL) || defined(USE_SMOOTHSTEP_ACCEL)
          if (m_position.distance < (ACCEL_DURATION << 1))
          {
            m_position.easein  = m_position.distance >> 1;
            m_position.easeout = m_position.easein;
          }
          else
          {
            m_position.easein  = ACCEL_DURATION;
            m_position.easeout = m_position.distance - ACCEL_DURATION;
          }
        #endif
      }
    #endif
  }
}


/**
 * @brief advance the motor one step, if appropriate
 * @details 
 * This method is called from the Interrupt Service Routine (ISR) for timer0, 
 * 
 * if the motor is moving, it issues a single step to the stepper motor driver, by delegating to derived classes.
 * This is either "clockwise" (cw, up) or "counter clockwise" (ccw, down), depending on the 
 * target position. 
 * 
 * The velocity of the movement is determined by the frequency with which the step routine is called.
 * The frequency is determined by 
 *  - m_set_speed, which is calculated from the chosen velocity profile (see config.h, in set_target_position())
 *  - m_speed, the speed that is set from the moonlite controller (on computer) or any keyboard/ui that is configured.
 *      This acts as a divider on the *_MAX_SPEED set in config.h, the larger the slower it is.
 * 
 * Two cascading counters are used, to determine the frequency with which steps are issued:
 *  - m_ovf_counter, this needs to overflow TIMER0_FREQ/(2*m_set_speed)
 *  - counter, which introduces a delay by needing to be 0 mod (m_speed/2). 
 * 
 * Note: The timer0 interrupt is configured in main (ardufocus.cpp), isr can be found in isr.h/isr.cpp
 */
void stepper::tick()
{
  // Movement guard
  if (! m_position.moving) { sleep(); return; }

  // Step frequency generator
  if((m_ovf_counter++) < (TIMER0_FREQ / (m_set_speed<<1)) -1) { return; }
  m_ovf_counter = 0;

  // Speed control
  // We're not 100% moonlite compatible here: the PPS speed value you select
  // on the driver acts as a divider of *_MAX_SPEED set on the config file.
  static uint8_t counter = 0;
  if((counter++) % (m_speed>>1) != 0) { return; }

  // Move outwards
  if (m_position.target > m_position.current) {
    if ((m_invert_direction) ? step_cw() : step_ccw()) { update_position(1); }
  }

  // Move inwards
  else if (m_position.target < m_position.current) {
    if ((m_invert_direction) ? step_ccw() : step_cw()) { update_position(-1); }
  }

  // Stop movement
  else { halt(); }
}


#ifdef HAS_ACCELERATION
  /**
   * @brief Determine the frequency with which steps are issued to the stepper motor driver.
   * @details 
   * Depending on the selected acceleration profile, set the current speed for the motor (m_set_speed).
   * If only an amount < ACCELL_MIN_STEPS needs to be moved, no acceleration profile is applied and m_min_speed is used.
   * If 
   */
  void stepper::update_freq()
  {
    if (m_position.distance >= ACCEL_MIN_STEPS)
    {
      #if defined(USE_LINEAR_ACCEL)
        const float s = (m_position.relative <= m_position.easein)
          ? m_position.relative : m_position.distance - m_position.relative;
        const float f = map(s, 0, m_position.easein, 0.0, 1.0);

      #elif defined(USE_TRAPEZOID_ACCEL)
        const float s = (m_position.relative <= m_position.easein)
          ? m_position.relative : (m_position.relative >= m_position.easeout)
          ? m_position.distance - m_position.relative : ACCEL_DURATION;
        const float f = map(s, 0, ACCEL_DURATION, 0.0, 1.0);

      #elif defined(USE_SMOOTHSTEP_ACCEL)
        const float f = util::smootheststep(0, m_position.easein, m_position.relative) *
          (1.0 - util::smootheststep(m_position.easeout, m_position.distance, m_position.relative));
      #endif

      m_set_speed = util::lerp(m_min_speed, m_max_speed, f);

    } else { m_set_speed = m_min_speed; }
  }
#endif

/**
 * @brief [brief description]
 * @details [long description]
 *
 */
void stepper::update_position(const int8_t &direction)
{
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    m_position.current += direction;          // Update the global position

    #ifdef HAS_ACCELERATION
      ++m_position.relative;  // Update the relative position
      update_freq();          // Update the stepping frequency
    #endif
  }
}
