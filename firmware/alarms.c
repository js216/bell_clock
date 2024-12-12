/*
 * Mini-library for alarms.
 *
 * Jakob Kastelic (Stanford Research Systems)
 */

#include "Arduino.h"
#include "alarms.h"

void alarm_check(alarm_t *alarm, const int h, const int m)
{
    // if alarm is armed, it goes off when it's time
    if (alarm->mode == ALARM_ARMED) {
        if ((h == alarm->hour) && (m == alarm->min)) {
            alarm->mode = ALARM_ACTIVE;
            alarm->counter = 0;
        }
    }

    // when alarm is in active state, count till we turn it off
    else if (alarm->mode == ALARM_ACTIVE) {
        alarm->counter++;

        // sound alarm at the ALARM_RATE
        if (alarm->counter % ALARM_RATE == ALARM_RATE-1) {
            // reset counter so it doesn't eventually overflow
            alarm->counter = 0;

            // issue bell pulse
            digitalWrite(alarm->ctl_pin, LOW);
            delay(BELL_DURATION);
            digitalWrite(alarm->ctl_pin, HIGH);

            // in one-shot mode, turn alarm off when it expires
            if (alarm->single)
                alarm->mode = ALARM_DONE;
        }
    }

    // when alarm is done, wait to re-arm
    else if (alarm->mode == ALARM_DONE) {
        if ((h != alarm->hour) || (m != alarm->min))
            alarm->mode = ALARM_ARMED;
    }
}


void alarm_cycle(alarm_t *alarm, const int dir, const int h, const int m)
{
    // cycle in the positive direction
    if (dir > 0) {
        if ((alarm->mode == ALARM_ARMED) || (alarm->mode == ALARM_DONE)) {
            if (alarm->single)
                alarm->single = false;
            else
                alarm->mode = ALARM_OFF;
        } else { // if not armed or done
            alarm->single = true;
            if ((h != alarm->hour) || (m != alarm->min))
                alarm->mode = ALARM_ARMED;
            else
                alarm->mode = ALARM_DONE;
        }
    }

    // cycle in the negative direction
    if (dir < 0) {
        if ((alarm->mode == ALARM_ARMED) || (alarm->mode == ALARM_DONE)) {
            if (alarm->single)
                alarm->mode = ALARM_OFF;
            else
                alarm->single = true;
        } else { // if not armed or done
            alarm->single = false;
            alarm->mode = ALARM_ARMED;
        }
    }

    // after cycling, SRAM data and LED state is out of date
    alarm->updated = false;
    alarm_led(alarm, true);
}


void alarm_led(alarm_t *alarm, const bool mode)
{
   if ((alarm->mode == ALARM_OFF) || (mode == false))
      for (int j=0; j<ALARM_NUM_COLORS; j++)
         digitalWrite(alarm->led_pin[j], LOW);
   else {
      if (alarm->single) {
         digitalWrite(alarm->led_pin[0], HIGH);
         digitalWrite(alarm->led_pin[1], LOW);
         digitalWrite(alarm->led_pin[2], LOW);
      } else {
         digitalWrite(alarm->led_pin[0], LOW);
         digitalWrite(alarm->led_pin[1], HIGH);
         digitalWrite(alarm->led_pin[2], LOW);
      }
   }
}

// end file alarms.c

