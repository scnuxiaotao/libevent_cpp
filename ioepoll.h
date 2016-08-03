/*
 * ioepoll.h
 *
 *  Created on: 2016年6月11日
 *      Author: xiaotao
 */

#ifndef IOEPOLL_H_
#define IOEPOLL_H_

class event_base;
class event;
class timeval;
class epoll_event;

struct evepoll {
	event *evread;
	event *evwrite;
};

struct epollop {
	evepoll *fds;
	int nfds;
	epoll_event *events;
	int nevents;
	int epfd;
};

#define INITIAL_NFILES 32
#define INITIAL_NEVENTS 32
#define MAX_NEVENTS 4096
#define MAX_EPOLL_TIMEOUT_MSEC (35*60*1000)

class io_epoll {
	friend class event_base;
public:
	io_epoll(event_base *base);
	virtual ~io_epoll();
private:
	int epoll_add(event *);
	int epoll_del(event *);
	int epoll_dispatch(timeval *);
private:
	epollop *epollop_;
	event_base *ev_base;
};

#endif /* IOEPOLL_H_ */
