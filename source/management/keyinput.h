#include "gba/hw_reg.h"

/**
 * poll hw_regs and update the results of the other two methods
 * Intended to be called once a frame
 */
void keyinput_read(void);

/**
 * Gets the buttons that are pressed this frame that were not pressed last frame
 * @note like the hw_regs, `true` means not pressed
 */
keypad_t keyinput_get_new(void);

/**
 * Gets the buttons that are pressed this frame
 * @note like the hw_regs, `true` means not pressed
 */
keypad_t keyinput_get_pressed(void);

int keyinput_horizontal_new(void);
int keyinput_horizontal_pressed(void);
int keyinput_vertical_new(void);
int keyinput_vertical_pressed(void);
int keyinput_shoulders_new(void);
int keyinput_shoulders_pressed(void);
