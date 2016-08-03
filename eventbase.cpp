/*
 * eventbase.cpp
 *
 *  Created on: 2016年6月11日
 *      Author: xiaotao
 */

#include "eventbase.h"
#include "event.h"
#include "ioepoll.h"
#include <iostream>
#include <stdlib.h>
#include "esignal.h"

#include <assert.h>
#include <cstring>

extern event_base *current_base; 
extern event_base *evsignal_base;

event_base::event_base() :
		evsel(NULL), event_count(0), event_count_active(0), activequeues(
		NULL), eventqueue(NULL), nactivequeues(0) {
	evsel = new io_epoll(this);                        
	event_base_priority_init(1);		
	eventqueue = new event_list();   
	TAILQ_INIT(this->eventqueue);	

	sig = new esignal(this);         

	if (current_base == NULL)
		current_base = this;
}

event_base::~event_base() {
	delete sig;
	delete evsel;
	delete eventqueue;

	for (int i = 0; i < nactivequeues; ++i) {
		delete activequeues[i];
	}

	delete activequeues;
}

int event_base::event_base_priority_init(int npriorities) {
	int i;

	if (event_count_active)  
		return -1;

	if (nactivequeues && npriorities != nactivequeues) {
		for (i = 0; i < nactivequeues; ++i) {
			delete[] activequeues[i];
		}
		delete[] activequeues;
	}

	nactivequeues = npriorities;
	activequeues = new event_list*[nactivequeues];

	if (activequeues == NULL) {
		exit(1);
	}

	memset(activequeues, 0, sizeof(*activequeues)*nactivequeues); 

	for (i = 0; i < nactivequeues; ++i) {
		activequeues[i] = new event_list();
		if (activequeues[i] == NULL) {
			exit(1);
		}
		TAILQ_INIT(activequeues[i]);
	}

	return 0;
}

int event_base::event_add(event *ev, const timeval *timeout) {
	int res = 0;

	if ((ev->ev_events & (EV_READ | EV_WRITE | EV_SIGNAL))
			&& !(ev->ev_flags & (EVLIST_INSERTED | EVLIST_ACTIVE))) {
		res = evsel->epoll_add(ev);
		if (res != -1)
			event_queue_insert(ev, EVLIST_INSERTED);
	}

	if (res != -1 && timeout != NULL) {
		timeval now;

		if (ev->ev_flags & EVLIST_TIMEOUT)
			event_queue_remove(ev, EVLIST_TIMEOUT);

		if ((ev->ev_flags & EVLIST_ACTIVE) && (ev->ev_res & EV_TIMEOUT)) {

			if (ev->ev_ncalls && ev->ev_pncalls) {
				*ev->ev_pncalls = 0;
			}

			event_queue_remove(ev, EVLIST_ACTIVE);
		}

		gettime(&now);
		evutil_timeradd(&now, timeout, &ev->ev_timeout);

		event_queue_insert(ev, EVLIST_TIMEOUT);
	}

	return res;
}

int event_base::event_haveevents() {
	return (event_count > 0);
}

int event_base::event_base_loop(int flags) {
	int res, done = 0;
	timeval tv;
	timeval *tv_p;

	tv_cache.tv_sec = 0;

	if (sig->ev_signal_added)
		evsignal_base = this;

	while (!done) {

		tv_p = &tv;
		if (!event_count_active && !(flags & EVLOOP_NONBLOCK)) {
			timeout_next(&tv_p);
		} else {
			evutil_timerclear(&tv);
		}

		if (!event_haveevents()) {
			return 1;
		}

		tv_cache.tv_sec = 0;

		res = evsel->epoll_dispatch(tv_p);    

		if (res == -1)
			return -1;

		gettime(&tv_cache);

		timeout_process();

		if (event_count_active) {
			event_process_active();
			if (!event_count_active && (flags & EVLOOP_ONCE))
				done = 1;
		} else if (flags & EVLOOP_NONBLOCK)
			done = 1;

	}
	tv_cache.tv_sec = 0;

	return 0;

}

void event_base::event_process_active() {
	event *ev;
	event_list *activeq = NULL;
	int i;
	short ncalls;

	for (i = 0; i < nactivequeues; ++i) {
		if (TAILQ_FIRST(activequeues[i]) != NULL) {
			activeq = activequeues[i];
			break;
		}
	}
	assert(activeq!=NULL);
	for (ev = TAILQ_FIRST(activeq); ev; ev = TAILQ_FIRST(activeq)) {
		if (ev->ev_events & EV_PERSIST)
			event_queue_remove(ev, EVLIST_ACTIVE);
		else
			event_del(ev);

		ncalls = ev->ev_ncalls;
		ev->ev_pncalls = &ncalls;
		while (ncalls) {
			ncalls--;
			ev->ev_ncalls = ncalls;
			(*ev->ev_callback)((int) ev->ev_fd, ev->ev_res, ev->ev_arg);
		}
	}
}

int event_base::event_del(event *ev) {
	event_base *base;
	io_epoll *evsel;

	if (ev->ev_base == NULL)
		return -1;

	base = ev->ev_base;
	evsel = base->evsel;

	if (ev->ev_ncalls && ev->ev_pncalls) {
		*ev->ev_pncalls = 0;
	}
	if (ev->ev_flags & EVLIST_TIMEOUT)
		event_queue_remove(ev, EVLIST_TIMEOUT);

	if (ev->ev_flags & EVLIST_ACTIVE)
		event_queue_remove(ev, EVLIST_ACTIVE);

	if (ev->ev_flags & EVLIST_INSERTED) {
		event_queue_remove(ev, EVLIST_INSERTED);
		return (evsel->epoll_del(ev));
	}

	return 0;
}

void event_base::event_queue_remove(event *ev, int queue) {
	if (!(ev->ev_flags & queue)) {   
		exit(1);
	}

	if (~ev->ev_flags & EVLIST_INTERNAL)
		event_count--;

	ev->ev_flags &= ~queue;   

	switch (queue) {
	case EVLIST_INSERTED:
		TAILQ_REMOVE(eventqueue, ev, ev_next);
		break;
	case EVLIST_ACTIVE:
		event_count_active--;
		TAILQ_REMOVE(activequeues[ev->ev_pri], ev, ev_active_next);
		break;
	case EVLIST_TIMEOUT:
		time_queue.pop();
		break;
	default:
		exit(1);
	}

}

void event_base::event_queue_insert(event *ev, int queue) {
	if (ev->ev_flags & queue) {
		if (queue & EVLIST_ACTIVE)
			return;
		exit(1);
	}

	if (~ev->ev_flags & EVLIST_INTERNAL) {
		event_count++;
	}

	ev->ev_flags |= queue;
	switch (queue) {
	case EVLIST_INSERTED:
		TAILQ_INSERT_TAIL(eventqueue, ev, ev_next);
		break;
	case EVLIST_ACTIVE:
		event_count_active++;
		TAILQ_INSERT_TAIL(activequeues[ev->ev_pri], ev, ev_active_next);
		break;
	case EVLIST_TIMEOUT: {
		time_queue.push(ev);
		break;
	}
	default:
		exit(1);
	}
}

void event_base::event_active(event *ev, int res, short ncalls) {
	if (ev->ev_flags & EVLIST_ACTIVE) {
		ev->ev_res |= res;
		return;
	}

	ev->ev_res = res;
	ev->ev_ncalls = ncalls;
	ev->ev_pncalls = NULL;
	event_queue_insert(ev, EVLIST_ACTIVE);
}

int event_base::event_get_nactivequeues() {
	return nactivequeues;
}

int event_base::gettime(timeval *tp) {
	if (tv_cache.tv_sec) {
		*tp = tv_cache;
		return 0;
	}

	timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
		return -1;

	tp->tv_sec = ts.tv_sec;
	tp->tv_usec = ts.tv_nsec / 1000;
	return 0;
}

int event_base::timeout_next(timeval **tv_p) {
	timeval now;
	event *ev;
	timeval *tv = *tv_p;

	if (time_queue.empty()) {
		*tv_p = NULL;
		return 0;
	}

	ev = time_queue.top();

	if (gettime(&now) == -1)
		return -1;

	if (evutil_timercmp(&ev->ev_timeout, &now, <=)) {
		evutil_timerclear(tv);
		return 0;
	}

	evutil_timersub(&ev->ev_timeout, &now, tv);

	assert(tv->tv_sec >= 0);
	assert(tv->tv_usec >= 0);

	return 0;
}
void event_base::timeout_process() {
	timeval now;
	event *ev;

	if (time_queue.empty())
		return;

	gettime(&now);

	while (!time_queue.empty()) {
		ev = time_queue.top();
		if (evutil_timercmp(&ev->ev_timeout, &now, >))
			break;

		event_del(ev);

		event_active(ev, EV_TIMEOUT, 1);
	}
}

