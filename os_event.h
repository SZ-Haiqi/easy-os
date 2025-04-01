#ifndef _OS_EVENT_H_
#define _OS_EVENT_H_

#include "os_type.h"

#define ENTER_CRITICAL()

#define EXIT_CRITICAL()

#define MAX_DELAY_QUEUE_SIZE 80
#define MAX_READY_QUEUE_SIZE 10

typedef struct {
	uint8_t event_group;
	uint8_t event_id;
	uint32_t delay_time;
} os_event_param_t;

typedef struct {
	os_event_param_t buf;
	bool used;
	os_node_t *next;
} os_node_t;

typedef struct {
	os_node_t sentinel_node; // 哨兵节点
	os_node_t *tail;	 // 尾指针（新增）
	uint8_t len;
} os_queue_t;

extern volatile uint64_t mcu_realtime_tick; // 单片机实时时间戳

bool os_event_create(uint8_t EventGroup, uint8_t EventId, uint32_t EventDelay);
uint8_t os_event_delete_by_id(uint8_t EventGroup, uint8_t EventId);
uint8_t os_event_deleted_group(uint8_t EventGroup);

void os_event_start(void);
void os_event_init(void);

#endif // _OS_EVENT_H_