/*
 * esignal.h
 *
 *  Created on: 2016年6月20日
 *      Author: xiaotao
 */

#ifndef ESIGNAL_H_
#define ESIGNAL_H_

#include "signal.h"
#include "event.h"
#include "eventbase.h"

typedef void (*ev_sighandler_t)(int);

class esignal {
	friend class event_base;
	friend class io_epoll;
private:
	esignal(event_base* ev_base);
	virtual ~esignal();
private:
	event ev_signal;
	int ev_signal_pair[2];
	int ev_signal_added;
	volatile sig_atomic_t evsignal_caught;
	event_list evsigevents[NSIG];
	sig_atomic_t evsigcaught[NSIG];
	ev_sighandler_t **sh_old;
	int sh_old_max;
	event_base* ev_base;
private:
	void evsignal_process();
	int evsignal_add(event *);
	int evsignal_del(event *);
	static void evsignal_cb(int fd, short what, void *arg);
	static void evsignal_handler(int sig);
	int _evsignal_set_handler(int evsignal,void (*handler)(int));
	int _evsignal_restore_handler(int evsignal);
};

#endif /* ESIGNAL_H_ */
