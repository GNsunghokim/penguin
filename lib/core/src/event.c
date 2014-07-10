#include <malloc.h>
#include <util/list.h>
#include <util/map.h>
#include <util/event.h>

typedef struct {
	EventFunc	func;
	void*		context;
} Node;

typedef struct {
	EventFunc	func;
	void*		context;
	clock_t		delay;
	clock_t		period;
} TimerNode;

typedef struct {
	int			event_id;
	TriggerEventFunc	func;
	void*			context;
} TriggerNode;

typedef struct {
	int			event_id;
	void*			event;
	TriggerEventFunc	last;
	void*			context;
} Trigger;

static List* busy_events;
static List* timer_events;
static Map* trigger_events;
static List* triggers;
static List* idle_events;

void event_init() {
	busy_events = list_create(malloc, free);
	timer_events = list_create(malloc, free);
	trigger_events = map_create(8, map_uint64_hash, map_uint64_equals, malloc, free);
	triggers = list_create(malloc, free);
	idle_events = list_create(malloc, free);
}

static bool is_trigger_stop;

static void fire(int event_id, void* event, TriggerEventFunc last, void* context) {
	List* list = map_get(trigger_events, (void*)(uint64_t)event_id);
	if(!list)
		return;
	
	ListIterator iter;
	list_iterator_init(&iter, list);
	while(list_iterator_has_next(&iter)) {
		TriggerNode* node = list_iterator_next(&iter);
		is_trigger_stop = false;
		if(!node->func(event_id, event, node->context)) {
			list_iterator_remove(&iter);
			free(node);
		}
		
		if(is_trigger_stop)
			return;
	}
	
	if(last)
		last(event_id, event, context);
}

static bool get_first_bigger(void* time, void* node) {
	return (uint64_t)time > ((TimerNode*)node)->delay;
}

static clock_t next_timer = INT64_MAX;

int event_loop() {
	int count = 0;
	
	// Busy events
	ListIterator iter;
	list_iterator_init(&iter, busy_events);
	while(list_iterator_has_next(&iter)) {
		Node* node = list_iterator_next(&iter);
		if(!node->func(node->context)) {
			list_iterator_remove(&iter);
			free(node);
		}
	}
	
	// Timer events
	clock_t time = clock();
	while(next_timer <= time) {
		TimerNode* node = list_remove_first(timer_events);
		if(node->func(node->context)) {
			node->delay += node->period;
			int index = list_index_of(timer_events, (void*)(uint64_t)node->delay, get_first_bigger);
			if(!list_add_at(timer_events, index, node)) {
				free(node);
				
				printf("Timer event lost unexpectedly cause of memory lack!!!\n");
				while(1) asm("hlt");
			}
		} else {
			free(node);
		}
		
		if(list_size(timer_events) > 0)
			next_timer = ((TimerNode*)list_get_first(timer_events))->delay;
		else
			next_timer = INT64_MAX;
		
		count++;
	}
	
	// Trigger events
	while(list_size(triggers) > 0) {
		Trigger* trigger = list_remove_first(triggers);
		fire(trigger->event_id, trigger->event, trigger->last, trigger->context);
		free(trigger);
		
		count++;
	}
	
	// Idle events
	if(list_size(idle_events) > 0) {
		Node* node = list_get_first(idle_events);
		if(!node->func(node->context)) {
			list_remove_first(idle_events);
			free(node);
		}
		
		list_rotate(idle_events);
		
		count++;
	}
	
	return count;
}

uint64_t event_busy_add(EventFunc func, void* context) {
	Node* node = malloc(sizeof(Node));
	if(!node)
		return 0;
	node->func = func;
	node->context = context;
	
	if(!list_add(busy_events, node)) {
		free(node);
		return 0;
	}
	
	return (uint64_t)node;
}

bool event_busy_remove(uint64_t id) {
	if(list_remove_data(busy_events, (void*)id)) {
		free((void*)id);
		return true;
	} else {
		return false;
	}
}

uint64_t event_timer_add(EventFunc func, void* context, clock_t delay, clock_t period) {
	TimerNode* node = malloc(sizeof(TimerNode));
	if(!node)
		return 0;
	node->func = func;
	node->context = context;
	clock_t time = clock();
	node->delay = time + delay;
	node->period = period;
	
	int index = list_index_of(timer_events, (void*)(uint64_t)node->delay, get_first_bigger);
	if(!list_add_at(timer_events, index, node)) {
		free(node);
		return 0;
	}
	
	next_timer = ((TimerNode*)list_get_first(timer_events))->delay;
	
	return (uint64_t)node;
}

bool event_timer_remove(uint64_t id) {
	if(list_remove_data(timer_events, (void*)id)) {
		free((void*)id);
		
		if(list_size(timer_events) > 0)
			next_timer = ((TimerNode*)list_get_first(timer_events))->delay;
		else
			next_timer = INT64_MAX;
		
		return true;
	} else {
		return false;
	}
}

uint64_t event_trigger_add(int event_id, TriggerEventFunc func, void* context) {
	TriggerNode* node = malloc(sizeof(TriggerNode));
	if(!node)
		return 0;
	node->event_id = event_id;
	node->func = func;
	node->context = context;
	
	List* list = map_get(trigger_events, (void*)(uint64_t)event_id);
	if(!list) {
		list = list_create(malloc, free);
		if(!list) {
			free(node);
			return 0;
		}
	}
	
	list_add(list, node);
	
	return (uint64_t)node;
}

bool event_trigger_remove(uint64_t id) {
	MapIterator iter;
	map_iterator_init(&iter, trigger_events);
	while(map_iterator_has_next(&iter)) {
		List* list = map_iterator_next(&iter)->data;
		if(list_remove_data(list, (void*)id)) {
			free((void*)id);
			
			if(list_size(list) == 0) {
				map_iterator_remove(&iter);
				list_destroy(list);
			}
			
			return true;
		}
	}
	
	return false;
}

void event_trigger_fire(int event_id, void* event, TriggerEventFunc last, void* context) {
	Trigger* trigger = malloc(sizeof(Trigger));
	if(!trigger) {
		fire(event_id, event, last, context);
		return;
	}
	trigger->event_id = event_id;
	trigger->event = event;
	trigger->last = last;
	trigger->context = context;
	
	if(!list_add(triggers, trigger)) {
		free(trigger);
		fire(event_id, event, last, context);
	}
}

void event_trigger_stop() {
	is_trigger_stop = true;
}

uint64_t event_idle_add(EventFunc func, void* context) {
	Node* node = malloc(sizeof(Node));
	if(!node)
		return 0;
	node->func = func;
	node->context = context;
	
	if(!list_add(idle_events, node)) {
		free(node);
		return 0;
	}
	
	return (uint64_t)node;
}

bool event_idle_remove(uint64_t id) {
	if(list_remove_data(idle_events, (void*)id)) {
		free((void*)id);
		return true;
	} else {
		return false;
	}
}