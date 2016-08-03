/*
 * eventbase.h
 *
 *  Created on: 2016年6月11日
 *      Author: xiaotao
 */

#ifndef EVENTBASE_H_
#define EVENTBASE_H_

#include <sys/queue.h>
#include<iostream>
#include <queue>
#include "event.h"

class io_epoll;
class event;
class esignal;

TAILQ_HEAD(event_list, event);

#define EVLOOP_ONCE	0x01	/**< Block at most once. */
#define EVLOOP_NONBLOCK	0x02	/**< Do not block. */

#define evutil_timeradd(tvp, uvp, vvp)							\
	do {														\
		(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;			\
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;       \
		if ((vvp)->tv_usec >= 1000000) {						\
			(vvp)->tv_sec++;									\
			(vvp)->tv_usec -= 1000000;							\
		}														\
	} while (0)
#define	evutil_timersub(tvp, uvp, vvp)						\
	do {													\
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
		if ((vvp)->tv_usec < 0) {							\
			(vvp)->tv_sec--;								\
			(vvp)->tv_usec += 1000000;						\
		}													\
	} while (0)
#define	evutil_timerclear(tvp)	(tvp)->tv_sec = (tvp)->tv_usec = 0
#define	evutil_timercmp(tvp, uvp, cmp)							\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?							\
	 ((tvp)->tv_usec cmp (uvp)->tv_usec) :						\
	 ((tvp)->tv_sec cmp (uvp)->tv_sec))

class event_base {
	friend class io_epoll;
	friend class esignal;
public:
	event_base();
	virtual ~event_base();

private:
	io_epoll *evsel;
	esignal* sig;
	int event_count;
	int event_count_active;
	event_list **activequeues;
	event_list *eventqueue;
	int nactivequeues;
	timeval tv_cache;
	class cmp_time {
	public:
		bool operator ()(const event* ev_a, const event* ev_b) {
			const timeval a = ev_a->ev_timeout, b = ev_b->ev_timeout;
			return ((a.tv_sec == b.tv_sec) ?
					(a.tv_usec > b.tv_usec) : (a.tv_sec > b.tv_sec));
		}
	};
	std::priority_queue<event*, std::vector<event*>, cmp_time> time_queue;

public:
	int event_add(event *ev, const timeval *timeout);
	int event_base_loop(int flags);
	int event_del(event *ev);
	int event_base_priority_init(int npriorities);
	int event_haveevents();
	int event_get_nactivequeues();
	int gettime(timeval *tp);

private:
	void event_active(event *ev, int res, short ncalls);
	void event_process_active();
	void event_queue_insert(event *ev, int queue);
	void event_queue_remove(event *ev, int queue);
	int timeout_next(timeval **tv_p);
	void timeout_process();
};

#endif /* EVENTBASE_H_ */
