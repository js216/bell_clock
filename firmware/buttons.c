/*
 * Mini-library for using pushbuttons.
 *
 * Jakob Kastelic (Stanford Research Systems)
 */

#include "Arduino.h"
#include "buttons.h"

void btn_init(btn_t *btn)
{
   pinMode(btn->pin, INPUT_PULLUP);
}


void btn_update(btn_t *btn)
{
   // detect short button presses
   if (digitalRead(btn->pin) != btn->old_state) {
      btn->old_state = digitalRead(btn->pin);
      if (btn->old_state == LOW) {
         btn->pressed = true;
         btn->cycles = 0;
      }
   }

   // count how long a button has been pressed
   if (digitalRead(btn->pin) == LOW)
      btn->cycles++;
   else {
      btn->cycles = 0;
      btn->longpressed = false;
   }

   // detect long button presses
   if (btn->cycles > BTN_LONGPRESS_DURATION)
      btn->longpressed = true;

   // when in a longpressed state, periodically emit retriggered presses
   if (btn->cycles % BTN_RETRIG_DURATION == BTN_RETRIG_DURATION-1)
      if (btn->longpressed) {
         btn->repressed = true;
         btn->cycles = 0;
      }
}


bool btn_was_pressed(btn_t *btn)
{
   const bool pressed = btn->pressed;
   btn->pressed = false;
   return pressed;
}


bool btn_was_longpressed(btn_t *btn)
{
   const bool longpressed = btn->longpressed;
   if (longpressed)
      btn->cycles = 0;
   btn->longpressed = false;
   return longpressed;
}


bool btn_was_repressed(btn_t *btn)
{
   const bool repressed = btn->repressed;
   btn->repressed = false;
   return repressed;
}


bool btn_two_pressed(btn_t *btn1, btn_t *btn2)
{
   if (btn1->pressed && btn2->pressed) {
      btn1->pressed = false;
      btn2->pressed = false;
      return true;
   } else
      return false;
}

bool btn_three_pressed(btn_t *btn1, btn_t *btn2, btn_t *btn3)
{
   if (btn1->pressed && btn2->pressed && btn3->pressed) {
      btn1->pressed = false;
      btn2->pressed = false;
      btn3->pressed = false;
      return true;
   } else
      return false;
}


// end file buttons.c

