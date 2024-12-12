/*
 * Mini-library for alarms.
 *
 * Jakob Kastelic (Stanford Research Systems)
 */

#ifndef ALARMS_h
#define ALARMS_h

#include <stdbool.h>

#define ALARM_NUM_COLORS 3
#define ALARM_RATE 1
#define BELL_DURATION 10

enum {
    ALARM_OFF,
    ALARM_ARMED,
    ALARM_ACTIVE,
    ALARM_DONE,
};

typedef struct {
    bool updated;   // when false, struct data is newer than SRAM data
    int hour;       // hour when alarm goes off
    int min;        // minute when alarm goes off
    int mode;       // alarm mode as per the alarm enum
    bool single;    // sound the bell once if true, repeatedly else
    int ctl_pin;    // pin used to control the alarm bell
    int led_pin[3]; // LED pins: red, green, blue
    int counter;    // used to sound the bell at a regular rate
} alarm_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check if an alarm is going off.
 *
 * @param alarm Struct with alarm information.
 * @param h Current hour.
 * @param m Current minute.
 */
void alarm_check(alarm_t *alarm, const int h, const int m);

/**
 * @brief Cycle through available alarm modes.
 *
 * @param alarm Struct with alarm information.
 * @param dif If positive, cycle Off->Single->Repeating, else cycle in the other
 * direction.
 * @param h Current hour.
 * @param m Current minute.
 */
void alarm_cycle(alarm_t *alarm, const int dir, const int h, const int m);

/**
 * @brief Turn the LED to the correct color depending on alarm mode.
 *
 * LED is off if alarm is off;
 * red if alarm is in single mode;
 * green if alarm is in continuous mode.
 *
 * @param alarm Struct with alarm information.
 * @param mode If false, turn all LEDs off. If true, turn the correct color on.
 */
void alarm_led(alarm_t *alarm, const bool mode);

#ifdef __cplusplus
}
#endif

#endif  // ALARMS_h

// end file alarms.h
