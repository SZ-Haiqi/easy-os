#ifndef _OS_EVENT_CUSTOMIZATION_H_
#define _OS_EVENT_CUSTOMIZATION_H_

#include "os_type.h"
#include "os_event.h"


typedef void (*Funtion)(uint8_t);

typedef enum {
	APP_CUSTOMIZATION_BASE_EVENT = 0x00,
        //add user new event group
        APP_CUSTOMIZATION_KEY_EVENT,
        APP_CUSTOMIZATION_HALL_EVENT,
        APP_CUSTOMIZATION_VBAT_EVENT,
        APP_CUSTOMIZATION_NTC_EVENT,
        //
	APP_CUSTOMIZATION_BASE_MAX
} customization_event_group;


typedef struct{
        customization_event_group Event_Group;
        Funtion EventGroup_CallBack;
}customization_event_callback_t;

extern customization_event_callback_t callback_table[APP_CUSTOMIZATION_BASE_MAX];
#endif