/*
 * event.cpp
 *
 *  Created on: 2016年6月11日
 *      Author: xiaotao
 */

#include "event.h"
#include "eventbase.h"

event_base *current_base = NULL;

event::event() {


}

event::event(int fd, short events,
		void (*callback)(int, short, void *), void *arg) {
	ev_base = current_base;
	ev_callback = callback;
	ev_fd = fd;
	ev_arg = arg;
	ev_events = events;
	ev_flags = EVLIST_INIT;
	ev_res = 0;
	ev_ncalls = 0;
	ev_pncalls = NULL;

	if(current_base)
			ev_pri = current_base->event_get_nactivequeues()/2;

}


void event::event_set(int fd, short events,
		void (*callback)(int, short, void *), void *arg) {
	ev_base = current_base;
	ev_callback = callback;
	ev_fd = fd;
	ev_arg = arg;
	ev_events = events;
	ev_flags = EVLIST_INIT;
	ev_res = 0;
	ev_ncalls = 0;
	ev_pncalls = NULL;

	if(current_base)
			ev_pri = current_base->event_get_nactivequeues()/2;
}

int event::event_base_set(event_base *base) {
	if (ev_flags != EVLIST_INIT)
		return -1;

	ev_base = base;

	return 0;

}

