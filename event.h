/*
 * event.h
 *
 *  Created on: 2016年6月11日
 *      Author: xiaotao
 */

#ifndef EVENT_H_
#define EVENT_H_


#include<iostream>


class event_base;


#ifndef TAILQ_ENTRY
#define TAILQ_ENTRY(type)			\
struct {												\
	struct type *tqe_next;				\
	struct type **tqe_prev;				\
}
#endif

#define EVLIST_TIMEOUT	0x01
#define EVLIST_INSERTED	0x02
#define EVLIST_SIGNAL	0x04
#define EVLIST_ACTIVE	0x08
#define EVLIST_INTERNAL	0x10
#define EVLIST_INIT	0x80

#define EV_TIMEOUT	0x01
#define EV_READ		0x02
#define EV_WRITE	0x04
#define EV_SIGNAL	0x08
#define EV_PERSIST	0x10


class event {
	friend class event_base;
	friend class io_epoll;
	friend class esignal;
public:
	event();
	event(int fd, short events,
			void (*callback)(int, short, void *), void *arg);

private:
	event_base *ev_base;        
	TAILQ_ENTRY (event) ev_active_next;            
	TAILQ_ENTRY (event) ev_next;                    
	TAILQ_ENTRY (event) ev_signal_next;             

	int ev_fd;

	void (*ev_callback)(int, short, void *arg);		
	int ev_flags;							
	short ev_events;
	short ev_ncalls;
	int ev_pri;
	short *ev_pncalls;
	int ev_res;
	void *ev_arg;
	timeval ev_timeout;

public:
	void event_set(int fd, short events,
			void (*callback)(int, short, void *), void *arg);
	int event_base_set(event_base *base);

};

#endif /* EVENT_H_ */
