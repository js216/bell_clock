/*
 * Mini-library for using pushbuttons.
 *
 * Jakob Kastelic (Stanford Research Systems)
 */

#ifndef BUTTONS_h
#define BUTTONS_h

#include <stdbool.h>

#define BTN_LONGPRESS_DURATION 12
#define BTN_RETRIG_DURATION    2

typedef struct {
    int  pin;         // the device pin the button is connected to
    bool old_state;   // for detecting if the state changed
    bool pressed;     // whether the button was pressed
    bool longpressed; // whether the button was longpressed
    bool repressed;   // when longpressed, retrigger continuously
    int  cycles;      // how many loop iterations it has been pressed for
} btn_t;

#ifdef __cplusplus
 extern "C" {
#endif

/**
 * @brief Initialize buttons as input pullups.
 */
void btn_init(btn_t *btn);

/**
 * @brief Read the button states.
 */
void btn_update(btn_t *btn);

/**
 * @brief Check if a button was pressed.
 *
 * @return True if it was, false if it was not.
 */
bool btn_was_pressed(btn_t *btn);

/**
 * @brief Check if a button was long-pressed.
 *
 * @return True if it was, false if it was not.
 */
bool btn_was_longpressed(btn_t *btn);

/**
 * @brief Check if a button was "re-pressed", i.e., when longpressed, it
 * continuously emits "repressed" signals.
 *
 * @return True if it was, false if it was not.
 */
bool btn_was_repressed(btn_t *btn);

/**
 * @brief Check if two buttons were pressed at the same time.
 */
bool btn_two_pressed(btn_t *btn1, btn_t *btn2);

/**
 * @brief Check if three buttons were pressed at the same time.
 */
bool btn_three_pressed(btn_t *btn1, btn_t *btn2, btn_t *btn3);

#ifdef __cplusplus
}
#endif

#endif // BUTTONS_h

// end file buttons.h

