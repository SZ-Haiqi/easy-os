#include "os_event.h"
#include "os_event_customization.h"
#include <stdlib.h>
os_node_t os_delay_queue_pool[MAX_DELAY_QUEUE_SIZE];
os_node_t os_ready_queue_pool[MAX_READY_QUEUE_SIZE];

os_queue_t os_delay_used_queue; //管理等待线程池内已使用的队列
os_queue_t os_delay_idle_queue; //管理等待线程池内未使用的队列

os_queue_t os_ready_used_queue; //管理就绪线程池内已使用的队列
os_queue_t os_ready_idle_queue; //管理就绪线程池内未使用的队列

volatile uint64_t os_ticks;	  // 系统时钟周期计数器
volatile uint64_t mcu_realtime_tick; // 单片机实时时间戳
/*****操作函数**************************************************************************************/
void _memcpy(void *dest, const void *src, uint8_t n)
{
	char *d = (char *)dest;
	const char *s = (const char *)src;
	while (n--)
		*d++ = *s++;
}

void _memset(void *s, int c, uint8_t n)
{
	unsigned char *d = (unsigned char *)s;
	while (n--)
		*d++ = (unsigned char)c;
}
/*****队列操作**************************************************************************************/
//把就绪区内的已使用队列的头节点提取出来，用来执行
os_node_t *acquire_ready_node_from_pool(void)
{
	os_node_t *get_node = NULL;
	if (os_ready_used_queue.len == 0)
		return NULL;

	get_node = os_ready_used_queue.sentinel_node.next;
	//如果是最后一个节点,需要将头节点和尾节点指向空
	if (get_node == os_ready_used_queue.tail) {
		os_ready_used_queue.tail = NULL;
		os_ready_used_queue.sentinel_node.next = NULL;
	} else {
		//更新新的头节点
		os_ready_used_queue.sentinel_node.next = get_node->next;
	}

	get_node->next = NULL;
	os_ready_used_queue.len--;

	return get_node;
}
//将节点放置到指定队列的队尾
void move_node_to_pool_tail(os_node_t *node, os_queue_t *pool, bool isInUse)
{
	if (node == NULL)
		return;
	node->next = NULL;
	node->used = isInUse;
	//移动游标，用于指向队尾
	if (pool->len == 0) {
		//头节点也是尾节点
		pool->sentinel_node.next = node;
		pool->tail = node;
	} else {
		pool->tail->next = node;
	}

	pool->tail = node;
	pool->len++;
}
//提取指定的空闲队列，将其头节点提取出来
os_node_t *have_node_to_idle_queue(os_queue_t *target_queue)
{
	if (target_queue->len == 0)
		return NULL;

	os_node_t *ret_node = target_queue->sentinel_node.next;
	//最后一个节点的处理
	if (ret_node == target_queue->tail) {
		target_queue->tail = NULL;
		target_queue->sentinel_node.next = NULL;
	} else {
		target_queue->sentinel_node.next = ret_node->next;
	}
	target_queue->len--;
	return ret_node;
}
//遍历等待区内的使用队列列表，将所有时间减1，若是已经归零了，那么将其放置到就绪区内的使用队列队尾；
void process_delayed_events(os_queue_t *delay_queue)
{
	if (delay_queue->len == 0)
		return;

	os_node_t *cursor = &delay_queue->sentinel_node;
	while (cursor->next) {
		os_node_t *current_node = cursor->next;
		if (current_node->buf.delay_time > 0) {
			current_node->buf.delay_time--;
			cursor = cursor->next; // 仅当未移动节点时前进游标
		} else {
			//从就绪区的空闲队列里提取头节点出来，用于移动到就绪区的使用队列的队尾
			os_node_t *ready_node = acquire_ready_node_from_pool();
			if (ready_node) {
				// 迁移数据到就绪队列
				ready_node->buf.delay_time = current_node->buf.delay_time;
				ready_node->buf.event_id = current_node->buf.event_id;
				ready_node->buf.event_group = current_node->buf.event_group;

				move_node_to_pool_tail(ready_node, &os_ready_used_queue, true);

				// 从延迟队列移除当前节点
				cursor->next = current_node->next; // 更新链表
				//将该节点放置到空闲队列的队尾，内包含了处理尾节点的状况
				move_node_to_pool_tail(current_node, &os_delay_idle_queue, false);
				delay_queue->len--;
				// 游标保持不动，继续检查新的 cursor->next
			} else {
				// 处理就绪队列满的情况
				cursor = cursor->next;
			}
		}
	}
}
/*****用户使用函数**********************************************************************************/
//创建一个新事件，放置到等待区的使用队列的队尾
bool os_event_create(uint8_t EventGroup, uint8_t EventId, uint32_t EventDelay)
{
	//提取等待区的空闲队列节点
	os_node_t *new = have_node_to_idle_queue(&os_delay_idle_queue);
	if (new == NULL)
		return false;

	new->buf.event_group = EventGroup;
	new->buf.event_id = EventId;
	new->buf.delay_time = EventDelay;
	new->next = NULL;
	new->used = false;
	//移动新节点到等待区的使用队列的队尾
	move_node_to_pool_tail(new, &os_delay_used_queue, true);
	return true;
}
//遍历等待区和就绪区的使用队列，查找符合条件的任务节点，将其设置为空闲并放置到对应的空闲队列的队尾
uint8_t os_event_delete_by_id(uint8_t EventGroup, uint8_t EventId)
{
	os_node_t *p_os_node_t = NULL;
	os_node_t *delet_node = NULL;
	uint8_t delet_num = 0;
	//先找就绪区的
	p_os_node_t = &os_ready_used_queue.sentinel_node;
	while (p_os_node_t->next) {
		os_node_t *current = p_os_node_t->next;
		if (current->buf.event_group == EventGroup && current->buf.event_id == EventId) {
			p_os_node_t->next = current->next;
			//如果是最后一个节点
			if (current->next == NULL) {
				os_ready_used_queue.tail = NULL;
			}
			os_ready_used_queue.len--;

			current->next = NULL;
			_memset(&current->buf, 0x00, sizeof(os_event_param_t));
			move_node_to_pool_tail(current, &os_ready_idle_queue, false);

			delet_num++;
		} else {
			p_os_node_t = current;
		}
	}
	//再找等待区的
	p_os_node_t = &os_delay_used_queue.sentinel_node;
	while (p_os_node_t->next) {
		os_node_t *current = p_os_node_t->next;
		if (current->buf.event_group == EventGroup && current->buf.event_id == EventId) {
			p_os_node_t->next = current->next;
			//如果是最后一个节点
			if (current->next == NULL) {
				os_delay_used_queue.tail = NULL;
			}
			os_delay_used_queue.len--;

			current->next = NULL;
			_memset(&current->buf, 0x00, sizeof(os_event_param_t));
			move_node_to_pool_tail(current, &os_ready_idle_queue, false);
			delet_num++;
		} else {
			p_os_node_t = current;
		}
	}
	return delet_num;
}
//遍历等待区和就绪区的使用队列，查找符合条件的任务节点，将其设置为空闲并放置到对应的空闲队列的队尾
uint8_t os_event_deleted_group(uint8_t EventGroup)
{
	os_node_t *p_os_node_t = NULL;
	os_node_t *delet_node = NULL;
	uint8_t delet_num = 0;
	//先找就绪区的
	p_os_node_t = &os_ready_used_queue.sentinel_node;
	while (p_os_node_t->next) {
		os_node_t *current = p_os_node_t->next;
		if (current->buf.event_group == EventGroup) {
			p_os_node_t->next = current->next;
			//如果是最后一个节点
			if (current->next == NULL) {
				os_ready_used_queue.tail = NULL;
			}
			os_ready_used_queue.len--;

			current->next = NULL;
			_memset(&current->buf, 0x00, sizeof(os_event_param_t));
			move_node_to_pool_tail(current, &os_ready_idle_queue, false);
			delet_num++;
		} else {
			p_os_node_t = current;
		}
	}
	//再找等待区的
	p_os_node_t = &os_delay_used_queue.sentinel_node;
	while (p_os_node_t->next) {
		os_node_t *current = p_os_node_t->next;
		if (current->buf.event_group == EventGroup) {
			p_os_node_t->next = current->next;
			//如果是最后一个节点
			if (current->next == NULL) {
				os_delay_used_queue.tail = NULL;
			}
			os_delay_used_queue.len--;

			current->next = NULL;
			_memset(&current->buf, 0x00, sizeof(os_event_param_t));
			move_node_to_pool_tail(current, &os_ready_idle_queue, false);
			delet_num++;
		} else {
			p_os_node_t = current;
		}
	}
	return delet_num;
}
//将执行的任务根据Event Group到对应的回调函数中执行
void os_event_function_execute(uint8_t EventGroup, uint8_t EventId)
{
	if (callback_table[EventGroup].Event_Group == EventGroup)
		callback_table[EventGroup].EventGroup_CallBack(EventId);
}
//无限循环函数，不断对节点进行操作
void os_event_start(void)
{
	for (;;) {
		os_node_t *execute_buf = acquire_ready_node_from_pool();
		if (execute_buf != NULL) {
			os_event_function_execute(execute_buf->buf.event_group, execute_buf->buf.event_id);
			//使用结束，列入空闲队列中去
			execute_buf->used = false;
			//清除原有的数据
			execute_buf->next = NULL;
			_memset(&execute_buf->buf, 0x00, sizeof(os_event_param_t));
			//移动到就绪列表的空闲队列的队尾
			move_node_to_pool_tail(execute_buf, &os_ready_idle_queue, false);
		}
		//中断产生
		if (os_ticks != mcu_realtime_tick) {
			ENTER_CRITICAL();
			os_ticks = mcu_realtime_tick;
			process_delayed_events(&os_delay_used_queue);
			EXIT_CRITICAL();
		}
	}
}
/*****初始化*****/
static void init_os_delay_IdleQueue(os_queue_t *pool, uint8_t capacity);

static void init_os_ready_IdleQueue(os_queue_t *pool, uint8_t capacity);

static void init_os_ready_delay_UsedQueue(os_queue_t *p_os_queue_t);
//初始化任务调度
void os_event_init(void)
{
	init_os_ready_delay_UsedQueue(&os_delay_used_queue);
	init_os_ready_delay_UsedQueue(&os_ready_used_queue);

	init_os_delay_IdleQueue(&os_delay_idle_queue, MAX_DELAY_QUEUE_SIZE);
	init_os_ready_IdleQueue(&os_ready_idle_queue, MAX_READY_QUEUE_SIZE);
}

/*****初始化***************************************************************************************/
//初始化等待区的空闲队列
static void init_os_delay_IdleQueue(os_queue_t *pool, uint8_t capacity)
{
	pool->len = capacity;
	pool->sentinel_node.next = &os_delay_queue_pool[0];

	for (uint8_t i = 0; i < capacity; i++) {
		if (i < capacity - 1)
			os_delay_queue_pool[i].next = &os_delay_queue_pool[i + 1];
		else {
			os_delay_queue_pool[i].next = NULL;
			pool->tail = &os_delay_queue_pool[i];
		}
		os_delay_queue_pool[i].used = false;
		_memset(&os_delay_queue_pool[i].buf, 0x00, sizeof(os_event_param_t));
	}
}
//初始化就绪区的空闲队列
static void init_os_ready_IdleQueue(os_queue_t *pool, uint8_t capacity)
{
	pool->len = capacity;
	pool->sentinel_node.next = &os_ready_queue_pool[0];

	for (uint8_t i = 0; i < capacity; i++) {
		if (i < capacity - 1)
			os_ready_queue_pool[i].next = &os_ready_queue_pool[i + 1];
		else {
			os_ready_queue_pool[i].next = NULL;
			pool->tail = &os_ready_queue_pool[i];
		}
		os_ready_queue_pool[i].used = false;
		_memset(&os_ready_queue_pool[i].buf, 0x00, sizeof(os_event_param_t));
	}
}
//初始化使用队列
static void init_os_ready_delay_UsedQueue(os_queue_t *p_os_queue_t)
{
	p_os_queue_t->len = 0;
	p_os_queue_t->sentinel_node.next = NULL;
	p_os_queue_t->tail = NULL;
}
/*****结束*****************************************************************************************/