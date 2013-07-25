//
//  lunchpad.h
//  lunchpad
//
//  Created by John Heaton on 3/26/13.
//  Copyright (c) 2013 John Heaton. All rights reserved.
//

#ifndef LUNCHPAD_H
#define LUNCHPAD_H

#include <CoreMIDI/CoreMIDI.h>
#include <stdint.h>

typedef struct _lp_device *lp_device_t;
typedef enum {
    lp_msg_type_note_on = 0,
    lp_msg_type_note_off = 1,
    lp_msg_type_scene_on = 2,
    lp_msg_type_scene_off = 3,
    lp_msg_type_top_on = 4,
    lp_msg_type_top_off = 5,
    lp_msg_type_unk = 0xFF
} lp_msg_type;

lp_device_t lp_get_device(); // iterate through online(connected) devices, returning first launchpad


// callback for incoming messages from launchpad
void lp_set_msg_cb(void (*msg_cb)(lp_device_t device, uint8_t data[256]));
void lp_enable_msg_cb(lp_device_t device);
void lp_disable_msg_cb(lp_device_t device);

int lp_is_new_launchpad_s(lp_device_t device);


// midi messages (outgoing). 0 == success
int lp_send_midi_msg(lp_device_t device, uint8_t data[256], size_t len);
int lp_activate_grid_pad_c(lp_device_t device, int column, int row, uint8_t red, uint8_t green); // values 0-3 for colors
int lp_activate_grid_pad_v(lp_device_t device, int column, int row, uint8_t velocity); // 0-127
int lp_activate_entire_grid_c(lp_device_t device, uint8_t red, uint8_t green);
int lp_activate_entire_grid_v(lp_device_t device, uint8_t velocity);
int lp_deactivate_entire_grid(lp_device_t device);
int lp_deactivate_grid_pad(lp_device_t device, int column, int row);

int lp_activate_top_pad_c(lp_device_t device, int index, uint8_t red, uint8_t green); // values 0-3 for colors
int lp_activate_top_pad_v(lp_device_t device, int index, uint8_t velocity); // 0-127
int lp_deactivate_top_pad(lp_device_t device, int index);


// misc
int lp_send_reset(lp_device_t device);
void lp_device_enable_feedback(lp_device_t device); // default is 127 velocity (bright yellow) ********* will only work for 8x8 grid, others you do manually
void lp_device_set_feedback_c(lp_device_t device, uint8_t red, uint8_t green);
void lp_device_set_feedback_v(lp_device_t device, uint8_t velocity);
void lp_device_disable_feedback(lp_device_t device);
// feedback will display when finger is down and then send a note-off on finger-up
// the msg callback is called after this happens so you can override its behavior if you have
// for example a background color that you wish to return to instead of turning the pad off


// message parsing
lp_msg_type lp_msg_get_type(uint8_t data[256]);
uint8_t lp_msg_get_velocity(uint8_t data[256]);
void lp_msg_get_grid_pad(uint8_t data[256], int *column, int *row);

#endif /* LUNCHPAD_H */
