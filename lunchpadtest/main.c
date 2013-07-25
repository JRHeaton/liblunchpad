//
//  main.c
//  lunchpadtest
//
//  Created by John Heaton on 3/27/13.
//  Copyright (c) 2013 John Heaton. All rights reserved.
//

#include <stdio.h>
#include "lunchpad.h"

//void msg_cb(lp_device_t d, uint8_t data[256]) {
//#define lz(x) case x: printf(#x "\n"); break;
//    int row, column;
//    lp_msg_get_grid_pad(data, &column, &row);
//    
//    if(lp_msg_get_type(data) == lp_msg_type_note_off)
//        lp_activate_grid_pad_v(d, column, row, 127);
//    
//    switch(lp_msg_get_type(data)) {
//            lz(lp_msg_type_note_on);
//            lz(lp_msg_type_note_off);
//            lz(lp_msg_type_scene_on);
//            lz(lp_msg_type_scene_off);
//            lz(lp_msg_type_top_off);
//            lz(lp_msg_type_top_on);
//    }
//}

int main(int argc, const char * argv[]) {
    lp_device_t d = lp_get_device();
//    lp_set_msg_cb(msg_cb);
//    lp_enable_msg_cb(d);
    
    lp_scrolling_text_fragment f[2] = {
        lp_stf("bo", 7),
        lp_stf("obs", 1)
    };
    
    lp_s_start_scrolling_text_v(d, f, 2, 31, 1);

//    lp_device_set_feedback_v(d, 31);
//    lp_device_enable_feedback(d);
    
//    lp_activate_entire_grid_v(d, 127);
    
    
    
    CFRunLoopRun();
}

