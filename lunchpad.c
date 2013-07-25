//
//  lunchpad.c
//  lunchpad
//
//  Created by John Heaton on 3/26/13.
//  Copyright (c) 2013 John Heaton. All rights reserved.
//

#include "lunchpad.h"

struct {
    MIDIClientRef client;
    MIDIPortRef inport, outport;
    void (*msg_cb)(lp_device_t device, uint8_t data[256]);
} _lp_ctx = { 0, 0, 0, NULL };

struct _lp_device {
    MIDIDeviceRef device;
    MIDIEndpointRef dest;
    int msg_cb_enabled;
    int fb_enabled;
    uint8_t red, green, velocity, type;
    uint8_t state[8][8];
    int is_lps;
#define _LPCOLOR 1
#define _LPVEL 0
};

typedef struct {
    uint8_t type;
    uint8_t key;
    uint8_t value;
} lp_midi_msg_t;

#define NOTE_ON 0x90
#define NOTE_OFF 0x80
#define CONT_CHNG 0xB0

lp_device_t _lp_create_device(MIDIDeviceRef d, MIDIEndpointRef dest, int msg_cb_enabled) {
    lp_device_t lp = calloc(1, sizeof(struct _lp_device));
    
    lp->device = d;
    lp->dest = dest;
    lp->msg_cb_enabled = msg_cb_enabled;
    lp->fb_enabled = 0;
    lp->red = lp->green = lp->type = 0;
    lp->velocity = 127;
    lp->is_lps = 0;
    
    return lp;
}

void _lp_midi_cb(const MIDINotification *message, void *refCon) {
    
}

void _lp_msg_in_cb(const MIDIPacketList *pktlist, void *readProcRefCon, void *srcConnRefCon) {
    lp_device_t device = (lp_device_t)srcConnRefCon;
    
    if(device->msg_cb_enabled && _lp_ctx.msg_cb) {
        const MIDIPacket *p = &pktlist->packet[0];
        for(int i=0;i<pktlist->numPackets;++i) {
            if(device->fb_enabled && p->data[0] == 0x90) {
                int row, column;
                lp_msg_get_grid_pad((uint8_t *)p->data, &column, &row);
                
                if(column >= 8)
                    goto sendcb;
                
                uint8_t *d = (uint8_t *)p->data;
                if(lp_msg_get_velocity(d) == 0) {
                    lp_deactivate_grid_pad(device, column, row);
                } else {
                    if(device->type == _LPCOLOR)
                        lp_activate_grid_pad_c(device, column, row, device->red, device->green);
                    else
                        lp_activate_grid_pad_v(device, column, row, device->velocity);
                }
            }
        sendcb:
            _lp_ctx.msg_cb(device, (uint8_t *)p->data);
            
            p = MIDIPacketNext(p);
        }
    }
}

void _lp_ctx_create() {
    if(!_lp_ctx.client)
        MIDIClientCreate(CFSTR("liblunchpad"), _lp_midi_cb, NULL, &_lp_ctx.client);
    
    if(!_lp_ctx.inport)
        MIDIInputPortCreate(_lp_ctx.client, CFSTR("liblunchpad-in"), _lp_msg_in_cb, NULL, &_lp_ctx.inport);
    
    if(!_lp_ctx.outport)
        MIDIOutputPortCreate(_lp_ctx.client, CFSTR("liblunchpad-out"), &_lp_ctx.outport);
}

int _lp_is_launchpad(MIDIDeviceRef d) {
    CFStringRef name;
    MIDIObjectGetStringProperty(d, kMIDIPropertyName, &name);

    return CFStringFind(name, CFSTR("Launchpad"), kCFCompareCaseInsensitive).length > 0;
}

void _lp_set_is_s(lp_device_t d) {
    CFStringRef name;
    MIDIObjectGetStringProperty(d->device, kMIDIPropertyName, &name);
    
    d->is_lps = CFStringFind(name, CFSTR("Launchpad S"), kCFCompareCaseInsensitive).length > 0;
}

int _lp_is_device_online(MIDIDeviceRef d) {
    SInt32 offline;
    
    MIDIObjectGetIntegerProperty(d, kMIDIPropertyOffline, &offline);
        
    return !offline;
}

int lp_is_new_launchpad_s(lp_device_t device) {
    return device->is_lps;
}

lp_device_t lp_get_device() {
    _lp_ctx_create();
    
    for(int i=0;i<MIDIGetNumberOfDevices();++i) {
        MIDIDeviceRef d = MIDIGetDevice(i);
        
        if(!_lp_is_launchpad(d) || !_lp_is_device_online(d))
            continue;
        
        MIDIEntityRef entity = MIDIDeviceGetEntity(d, 0);
        MIDIEndpointRef source = MIDIEntityGetSource(entity, 0);
        MIDIEndpointRef dest = MIDIEntityGetDestination(entity, 0);
        
        lp_device_t device = _lp_create_device(d, dest, 0);
        MIDIPortConnectSource(_lp_ctx.inport, source, device);
        
        _lp_set_is_s(device);
        lp_send_reset(device);
        
        return device;
    }
    
    return NULL;
}

void lp_set_msg_cb(void (*msg_cb)(lp_device_t device, uint8_t data[256])) {
    _lp_ctx.msg_cb = msg_cb;
}

void lp_enable_msg_cb(lp_device_t device) {
    device->msg_cb_enabled = 1;
}

void lp_disable_msg_cb(lp_device_t device) {
    device->msg_cb_enabled = 0;
}

int lp_activate_entire_grid_c(lp_device_t device, uint8_t red, uint8_t green) {
    lp_midi_msg_t msg;
    
    for(int i=0;i<8;++i) {
        for(int r=0;r<8;++r) {
            msg.type = NOTE_ON;
            msg.key = (0x10 * r) + i;
            msg.value = (0x10 * green) + red + 0x0C;
            
            if(lp_send_midi_msg(device, (uint8_t *)&msg, 3) != 0)
                return -1;
        }
    }
    
    return 0;
}

int lp_activate_entire_grid_v(lp_device_t device, uint8_t velocity) {
    lp_midi_msg_t msg;
    
    for(int i=0;i<8;++i) {
        for(int r=0;r<8;++r) {
            msg.type = NOTE_ON;
            msg.key = (0x10 * r) + i;
            msg.value = velocity;
            
            if(lp_send_midi_msg(device, (uint8_t *)&msg, 3) != 0)
                return -1;
        }
    }
    
    return 0;
}

int lp_s_start_scrolling_text_v(lp_device_t device, lp_scrolling_text_fragment *fragments, int fragment_count, uint8_t velocity, int loop) {
    if(!lp_is_new_launchpad_s(device))
        return -1;
    
    size_t full_text_len = 0;
    for(int i=0;i<fragment_count;++i) {
        lp_scrolling_text_fragment f = fragments[i];
        
        full_text_len += strlen(f.text);
    }
    
    size_t full_buf_len = 7 + full_text_len + fragment_count;
    uint8_t *buf = calloc(1, full_buf_len);
    
    // sysex shit
    buf[0] = 0xf0;
    buf[1] = 0x00;
    buf[2] = 0x20;
    buf[3] = 0x29;
    buf[4] = 0x09;
    
    if(loop)
        velocity |= 0x40; // loop bit
    buf[5] = velocity;
    
#define _fix_spd(s) (s > 7 ? 7 : (s < 1 ? 1 : s))
    
    int cind = 6;
    
    for(int i=0;i<fragment_count;++i) {
        lp_scrolling_text_fragment f = fragments[i];
        
        buf[cind] = _fix_spd(f.speed);
        strcpy((void *)&buf[cind+1], f.text);
        
        cind += strlen(f.text) + 1;
    }
    
    buf[6 + full_text_len + fragment_count] = 0xf7;
    
    return lp_send_midi_msg(device, buf, full_buf_len);
}

int lp_s_stop_scrolling_text(lp_device_t device) {
    uint8_t buf[6];
    
    buf[0] = 0xf0;
    buf[1] = 0x00;
    buf[2] = 0x20;
    buf[3] = 0x29;
    buf[4] = 0x09;
    buf[5] = 0xf7;
    
    return lp_send_midi_msg(device, buf, 6);
}

int lp_deactivate_entire_grid(lp_device_t device) {
    lp_midi_msg_t msg;
    
    for(int i=0;i<8;++i) {
        for(int r=0;r<8;++r) {
            msg.type = NOTE_OFF;
            msg.key = (0x10 * r) + i;
            msg.value = 0;
            
            if(lp_send_midi_msg(device, (uint8_t *)&msg, 3) != 0)
                return -1;
        }
    }
    
    return 0;
}

int lp_send_midi_msg(lp_device_t device, uint8_t data[256], size_t len) {
    uint8_t buf[256];
    bzero(buf, 256);
    
    MIDIPacketList *list = (MIDIPacketList *)buf;
    (void)MIDIPacketListInit(list);
    
    list->numPackets = 1;
    list->packet[0].length = len;
    list->packet[0].timeStamp = 0;
    memcpy(list->packet[0].data, data, len);
    
    return MIDISend(_lp_ctx.outport, device->dest, (const MIDIPacketList *)list);
}

int lp_activate_grid_pad_c(lp_device_t device, int column, int row, uint8_t red, uint8_t green) {
    lp_midi_msg_t msg;
    
    msg.type = NOTE_ON;
    msg.key = (0x10 * row) + column;
    msg.value = (0x10 * green) + red + 0x0C;
        
    return lp_send_midi_msg(device, (uint8_t *)&msg, 3);
}

int lp_activate_grid_pad_v(lp_device_t device, int column, int row, uint8_t velocity) {
    lp_midi_msg_t msg;
    
    msg.type = NOTE_ON;
    msg.key = (0x10 * row) + column;
    msg.value = velocity;
        
    return lp_send_midi_msg(device, (uint8_t *)&msg, 3);
}

int lp_deactivate_grid_pad(lp_device_t device, int column, int row) {
    lp_midi_msg_t msg;
    
    msg.type = NOTE_OFF;
    msg.key = (0x10 * row) + column;
    msg.value = 0;
        
    return lp_send_midi_msg(device, (uint8_t *)&msg, 3);
}

int lp_activate_top_pad_c(lp_device_t device, int index, uint8_t red, uint8_t green) {
    lp_midi_msg_t msg;
    
    msg.type = CONT_CHNG;
    msg.key = 0x68 + index;
    msg.value = (0x10 * green) + red + 0x0C;
    
    return lp_send_midi_msg(device, (uint8_t *)&msg, 3);
}

int lp_activate_top_pad_v(lp_device_t device, int index, uint8_t velocity) {
    lp_midi_msg_t msg;
    
    msg.type = CONT_CHNG;
    msg.key = 0x68 + index;
    msg.value = velocity;
    
    return lp_send_midi_msg(device, (uint8_t *)&msg, 3);
}

int lp_deactivate_top_pad(lp_device_t device, int index) {
    lp_midi_msg_t msg;
    
    msg.type = CONT_CHNG;
    msg.key = 0x68 + index;
    msg.value = 0;
    
    return lp_send_midi_msg(device, (uint8_t *)&msg, 3);
}

int lp_send_reset(lp_device_t device) {
    lp_midi_msg_t msg;
    
    msg.type = CONT_CHNG;
    msg.key = 0; 
    msg.value = 0;
    
    return lp_send_midi_msg(device, (uint8_t *)&msg, 3);
}

lp_msg_type lp_msg_get_type(uint8_t data[256]) {
    int v = lp_msg_get_velocity(data);
    
    switch(data[0]) {
        case NOTE_ON: {
            int column, row;
            lp_msg_get_grid_pad(data, &column, &row);
            
            if(column >= 8)
                return v == 0 ? lp_msg_type_scene_off : lp_msg_type_scene_on;
            else
                return v == 0 ? lp_msg_type_note_off : lp_msg_type_note_on;
        } break;
            
        case CONT_CHNG: {
            if(data[1] >= 0x68 && data[1] <= 0x6F)
                return v == 0 ? lp_msg_type_top_off : lp_msg_type_top_on;
        } break;
    }
    
    return lp_msg_type_unk;
}

uint8_t lp_msg_get_velocity(uint8_t data[256]) {
    return data[2];
}

void lp_msg_get_grid_pad(uint8_t data[256], int *column, int *row) {
    *column = (data[1] & 0x0F);
    *row = (data[1] & 0xF0) >> 4;
}

void lp_device_enable_feedback(lp_device_t device) {
    device->fb_enabled = 1;
}

void lp_device_set_feedback_c(lp_device_t device, uint8_t red, uint8_t green) {
    device->red = red;
    device->green = green;
    device->type = _LPCOLOR;
}

void lp_device_set_feedback_v(lp_device_t device, uint8_t velocity) {
    device->velocity = velocity;
    device->type = _LPVEL;
}

void lp_device_disable_feedback(lp_device_t device) {
    device->fb_enabled = 0;
}