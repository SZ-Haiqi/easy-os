#include "os_event_customization.h"

customization_event_callback_t callback_table[APP_CUSTOMIZATION_BASE_MAX] = {
	//example
	{
		.Event_Group = APP_CUSTOMIZATION_BASE_EVENT,
		.EventGroup_CallBack = NULL,
	},
};
