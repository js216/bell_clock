/*
 * Simple three-button alarm clock.
 *
 * Jakob Kastelic, Stanford Research Systems.
 */

// Arduino libraries
#include <Wire.h>
#include <SPI.h>

// vendor-specific libraries
#include <Adafruit_GFX.h>
#include "SparkFunDS1307RTC.h"
#include "Adafruit_LEDBackpack.h"

// this application
#include "buttons.h"
#include "alarms.h"

// compile-time parameters
#define NUM_BTNS 3
#define NUM_ALARMS 3
#define SLOW_FLASHING 16
#define FAST_FLASHING 6
#define SUPER_FAST_FLASHING 3
#define SEC_DISPLAY_DURATION 5
#define OFFS_REG 12
#define SRAM_OFFS 8
#define OFFSET_TO_MIN 433

// modes for the state machine
enum {
   MODE_NORMAL,
   MODE_EDIT_ALARM,
   MODE_EDIT_TIME,
   MODE_DISPLAY_SECONDS,
   MODE_EDIT_XTAL,
};

// pin definitions
const int led_pin[][ALARM_NUM_COLORS] = {{5, A7, A2}, {7, 1, A1}, {9, 0, 8}};
const int btn_pin[] = {4, 6, A0};
const int buzz_pin = 3;
const int rtc_sqw_pin = A6;
const int ctl_pin = 10;

// global variables
volatile bool second_passed;
int hour, minute, second;
int mode, edit_sel;
int disp_sec_countdown;

// for offset correction
int8_t offset_interval;
bool offset_sign;
int min_to_correction;

// modules
Adafruit_7segment matrix = Adafruit_7segment();
btn_t btn_arr[NUM_BTNS];
alarm_t alarm_arr[NUM_ALARMS];

/**
 * Interrupt routine when the square wave pin gets a transition.
 */
void sqw_irq(void)
{
   second_passed = true;
}


void setup() {
   // buzzer
   pinMode(buzz_pin, OUTPUT);

   // setup RTC (SparkFun BOB-10160 using DS3234)
   rtc.begin();
   rtc.enable();
   rtc.set24Hour(true);
   rtc.update();
   rtc.writeSQW(SQW_SQUARE_1);
   pinMode(rtc_sqw_pin, INPUT_PULLUP);
   attachInterrupt(digitalPinToInterrupt(rtc_sqw_pin), sqw_irq, RISING);
   second = rtc.second();
   minute = rtc.minute();
   hour = rtc.hour();

   // offset correction
   offset_interval = rtc.i2cReadByte(DS1307_RTC_ADDRESS, SRAM_OFFS+OFFS_REG);
   min_to_correction = OFFSET_TO_MIN * abs(offset_interval);

   // setup LEDs
   for (int i=0; i<NUM_BTNS; i++)
      for (int j=0; j<ALARM_NUM_COLORS; j++)
         if (j == 0)
            pinMode(led_pin[i][j], OUTPUT);
         else // for green & blue, use pullup resistor to make them less bright
            pinMode(led_pin[i][j], INPUT);

   // setup buttons
   for (int i=0; i<NUM_BTNS; i++) {
      btn_arr[i].pin = btn_pin[i];
      btn_init(&(btn_arr[i]));
   }

   // setup display (Adafruit 3108 using HT16K33 driver)
   matrix.begin(0x70);
   matrix.setBrightness(0);

   // setup alarms
   pinMode(ctl_pin, OUTPUT);
   digitalWrite(ctl_pin, HIGH);
   for (int i=0; i<NUM_ALARMS; i++) {
      alarm_arr[i].hour    = rtc.i2cReadByte(DS1307_RTC_ADDRESS, SRAM_OFFS+4*i);
      alarm_arr[i].min     = rtc.i2cReadByte(DS1307_RTC_ADDRESS, SRAM_OFFS+4*i+1);
      alarm_arr[i].mode    = rtc.i2cReadByte(DS1307_RTC_ADDRESS, SRAM_OFFS+4*i+2);
      alarm_arr[i].single  = rtc.i2cReadByte(DS1307_RTC_ADDRESS, SRAM_OFFS+4*i+3);
      alarm_arr[i].ctl_pin = ctl_pin;
      for (int j=0; j<ALARM_NUM_COLORS; j++)
         alarm_arr[i].led_pin[j] = led_pin[i][j];
      alarm_led(&(alarm_arr[i]), true);
   }
}


void loop() {
   // every second
   if (second_passed) {
      // clear flag
      second_passed = false;

      // offset correction
      int dt = 1;
      if (second == 0) {
         if (min_to_correction == 0) {
            if (offset_interval > 0)
               dt = 2; // insert a second
            else if (offset_interval < 0)
               dt = 0; // skip a second
            min_to_correction = OFFSET_TO_MIN * abs(offset_interval);
         } else {
            dt = 1;
            min_to_correction--;
         }
      }

      // advance time
      if (second+dt >= 60) {
         if (minute == 59)
            hour = (hour + 1) % 24;
         minute = (minute + 1) % 60;
      }
      second = (second + dt) % 60;

      // record offset correction to RTC
      if (dt != 1) {
         rtc.setHour(hour);
         rtc.setMinute(minute);
         rtc.setSecond(second);
      }

      // make a clicking sound
      digitalWrite(buzz_pin, HIGH);
      digitalWrite(buzz_pin, LOW);

      // check alarms
      for (int i=0; i<NUM_ALARMS; i++)
         alarm_check(&(alarm_arr[i]), hour, minute);

      // exit "display seconds" mode when countdown expires
      if (disp_sec_countdown >= 1)
         disp_sec_countdown--;
      if (disp_sec_countdown == 1)
         mode = MODE_NORMAL;

      // flash the colon on the display
      static bool display_colon;
      display_colon = !display_colon;
      matrix.drawColon(display_colon);
      matrix.writeDisplay();
   }

   // update alarms, display, and buttons
   sram_update();
   display_flash();
   display_update();
   check_buttons();

   // loop delay
   delay(50);
}


void sram_update(void)
{
   for (int i=0; i<NUM_ALARMS; i++) {
      if (! (alarm_arr[i].updated)) {
         rtc.i2cWriteByte(DS1307_RTC_ADDRESS, SRAM_OFFS+4*i,   (uint8_t)alarm_arr[i].hour);
         rtc.i2cWriteByte(DS1307_RTC_ADDRESS, SRAM_OFFS+4*i+1, (uint8_t)alarm_arr[i].min);
         rtc.i2cWriteByte(DS1307_RTC_ADDRESS, SRAM_OFFS+4*i+2, (uint8_t)alarm_arr[i].mode);
         rtc.i2cWriteByte(DS1307_RTC_ADDRESS, SRAM_OFFS+4*i+3, (uint8_t)alarm_arr[i].single);
      }
   }
}


void display_flash(void)
{
   // do not flash in normal mode
   if (mode == MODE_NORMAL)
      return;

   // determine flashing rate
   unsigned int rate = -1;
   if (mode == MODE_EDIT_ALARM)
      rate = SLOW_FLASHING;
   else if (mode == MODE_EDIT_TIME)
      rate = FAST_FLASHING;
   else if (mode == MODE_EDIT_XTAL)
      rate = SUPER_FAST_FLASHING;

   // counter to control screen flashing
   static unsigned int loop_iteration;
   loop_iteration++;

   // flash the screen and LEDs
   if (loop_iteration % rate == rate-1) {
      loop_iteration = 0;
      matrix.setBrightness(5);
      if (mode == MODE_EDIT_ALARM)
         alarm_led(&(alarm_arr[edit_sel]), true);
   } else if (loop_iteration % (rate/2) == (rate/2)-1) {
      matrix.setBrightness(0);
      if (mode == MODE_EDIT_ALARM)
         alarm_led(&(alarm_arr[edit_sel]), false);
   }
}


void display_update(void)
{
   // to keep track when display needs updating
   static int old_num=-1;
   int new_num=-1;

   // determine what to display
   if ((mode == MODE_NORMAL) || (mode == MODE_EDIT_TIME))
      new_num = hour*100 + minute;
   else if (mode == MODE_EDIT_ALARM)
      new_num = alarm_arr[edit_sel].hour*100 + alarm_arr[edit_sel].min;
   else if (mode == MODE_DISPLAY_SECONDS)
      new_num = minute*100 + second;
   else if (mode == MODE_EDIT_XTAL) {
      new_num = offset_interval;
   } else
      new_num = 0;

   // sanity check the number
   if (new_num > 9999)
      new_num = 9999;
   else if (new_num < -999)
      new_num = -999;

   // update display if needed
   if (old_num != new_num) {
      // write digits
      if (mode == MODE_EDIT_XTAL)
         matrix.println(new_num);
      else {
         matrix.writeDigitNum(0, (new_num/1000)%10, false);
         matrix.writeDigitNum(1, (new_num/100 )%10, false);
         matrix.writeDigitNum(3, (new_num/10  )%10, false);
         matrix.writeDigitNum(4, (new_num/1   )%10, false);
      }

      // flush data to display
      matrix.writeDisplay();
      old_num = new_num;
   }
}


void check_buttons(void)
{
   // check if buttons were pressed
   for (int i=0; i<NUM_BTNS; i++)
      btn_update(&(btn_arr[i]));

   // if any button pressed, silence all active alarms
   for (int i=0; i<NUM_BTNS; i++)
      if (btn_arr[i].pressed) {
         for (int j=0; j<NUM_ALARMS; j++)
            if (alarm_arr[i].mode == ALARM_ACTIVE)
               alarm_arr[i].mode = ALARM_DONE;
         break;
      }

   // button presses in normal mode
   if (mode == MODE_NORMAL) {
      for (int i=0; i<NUM_BTNS; i++) {
         // check if all three buttns were pressed at the same time
         if (btn_three_pressed(&(btn_arr[0]), &(btn_arr[1]),
                  &(btn_arr[2]))) {
            detachInterrupt(digitalPinToInterrupt(rtc_sqw_pin));
            rtc.writeSQW(SQW_SQUARE_4K);
            mode = MODE_EDIT_XTAL;
         }

         // check if buttons 0 & 1 were pressed at the same time
         else if (btn_two_pressed(&(btn_arr[0]), &(btn_arr[1])))
            mode = MODE_EDIT_TIME;

         // check if buttons 1 & 2 were pressed at the same time
         else if (btn_two_pressed(&(btn_arr[1]), &(btn_arr[2]))) {
            disp_sec_countdown = (SEC_DISPLAY_DURATION + 1);
            mode = MODE_DISPLAY_SECONDS;
         }

         // short button press: cycle alarm modes
         else if (btn_was_pressed(&(btn_arr[i]))) {
            alarm_cycle(&(alarm_arr[i]), +1, hour, minute);
         }

         // long button press: enter alarm edit mode
         else if (btn_was_longpressed(&(btn_arr[i]))) {
            btn_was_repressed(&(btn_arr[i]));
            alarm_cycle(&(alarm_arr[i]), -1, hour, minute);
            mode = MODE_EDIT_ALARM;
            edit_sel = i;
         }
      }
   }

   // button presses in alarm edit mode
   else if (mode == MODE_EDIT_ALARM) {
      // short (or held) press on button 0: increment hours
      if ((btn_was_pressed(&(btn_arr[0])))
            || (btn_was_repressed(&(btn_arr[0])))) {
         alarm_arr[edit_sel].updated = false;
         alarm_arr[edit_sel].hour = (alarm_arr[edit_sel].hour + 1) % 24;
      }

      // short (or held) press on button 1: increment minutes
      if ((btn_was_pressed(&(btn_arr[1])))
            || (btn_was_repressed(&(btn_arr[1])))) {
         alarm_arr[edit_sel].updated = false;
         alarm_arr[edit_sel].min = (alarm_arr[edit_sel].min + 1) % 60;
      }

      // short press on button 2: exit edit mode
      if (btn_was_pressed(&(btn_arr[2]))) {
         alarm_led(&(alarm_arr[edit_sel]), true);
         matrix.setBrightness(0);
         mode = MODE_NORMAL;
      }
   }

   // button presses in time edit mode
   else if (mode == MODE_EDIT_TIME) {
      // short (or held) press btn 0: increment hours
      if ((btn_was_pressed(&(btn_arr[0])))
            || (btn_was_repressed(&(btn_arr[0])))) {
         hour = (hour + 1) % 24;
         rtc.setHour(hour);
      }

      // short (or held) press btn 1: increment min & zero the sec
      if ((btn_was_pressed(&(btn_arr[1])))
            || (btn_was_repressed(&(btn_arr[1])))) {
         minute = (minute + 1) % 60;
         second = 0;
         rtc.setMinute(minute);
         rtc.setSecond(second);
         min_to_correction = OFFSET_TO_MIN * abs(offset_interval);
      }

      // short press on button 2: exit edit mode
      if (btn_was_pressed(&(btn_arr[2]))) {
         matrix.setBrightness(0);
         mode = MODE_NORMAL;
      }
   }

   // button presses in xtal aging offset edit mode
   else if (mode == MODE_EDIT_XTAL) {
      // short (or held) press btn 0: decrement offset
      if ((btn_was_pressed(&(btn_arr[0])))
            || (btn_was_repressed(&(btn_arr[0]))))
         offset_interval--;

      // short (or held) press btn 1: increment offset
      if ((btn_was_pressed(&(btn_arr[1])))
            || (btn_was_repressed(&(btn_arr[1]))))
         offset_interval++;

      // short press on button 2: exit edit xtal mode
      if (btn_was_pressed(&(btn_arr[2]))) {
         // write offset to register
         rtc.i2cWriteByte(DS1307_RTC_ADDRESS, SRAM_OFFS+OFFS_REG, offset_interval);
         min_to_correction = OFFSET_TO_MIN * abs(offset_interval);

         // update time from RTC
         rtc.update();
         second = rtc.second();
         minute = rtc.minute();
         hour = rtc.hour();

         // stop flashing the display
         matrix.setBrightness(0);

         // move to normal mode
         mode = MODE_NORMAL;

         // reattach "per second" interrupt
         rtc.writeSQW(SQW_SQUARE_1);
         attachInterrupt(digitalPinToInterrupt(rtc_sqw_pin), sqw_irq, FALLING);
      }
   }
}

// end file clock2.ino

