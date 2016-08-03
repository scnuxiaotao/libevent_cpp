/*
 * esignal.cpp
 *
 *  Created on: 2016年6月20日
 *      Author: xiaotao
 */

#include "esignal.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/queue.h>
#include "errno.h"
#include <stdlib.h>
#include <cstring>
#include <unistd.h>

#define FD_CLOSEONEXEC(x) do { \
        if (fcntl(x, F_SETFD, 1)) ; \
} while (0)

#define SOCKET_NONBLOCK(x) do { \
		if (fcntl(x, F_SETFL, O_NONBLOCK) == -1) ; \
} while (0)

event_base *evsignal_base = NULL;

esignal::esignal(event_base* base) {
	int i;

	ev_base = base;
	ev_signal_pair[0] = -1;
	ev_signal_pair[1] = -1;
	ev_signal_added = 0;
	if (socketpair(
	AF_UNIX, SOCK_STREAM, 0, ev_signal_pair) == -1) {
		exit(1);
	}

	FD_CLOSEONEXEC(ev_signal_pair[0]);
	FD_CLOSEONEXEC(ev_signal_pair[1]);
	sh_old = NULL;
	sh_old_max = 0;
	evsignal_caught = 0;
	memset(&evsigcaught, 0, sizeof(sig_atomic_t) * NSIG);

	for (i = 0; i < NSIG; ++i)
		TAILQ_INIT(&evsigevents[i]);

	SOCKET_NONBLOCK(ev_signal_pair[0]);

	ev_signal.event_set(ev_signal_pair[1],
	EV_READ | EV_PERSIST, esignal::evsignal_cb, &ev_signal);
	ev_signal.ev_base = base;
	ev_signal.ev_flags |= EVLIST_INTERNAL;

}
esignal::~esignal() {

	int i = 0;

	if (ev_signal_added) {
		ev_base->event_del(&ev_signal);
		ev_signal_added = 0;
	}
	for (i = 0; i < NSIG; ++i) {
		if (i < sh_old_max && sh_old[i] != NULL)
			_evsignal_restore_handler(i);
	}

	close(ev_signal_pair[0]);
	ev_signal_pair[0] = -1;
	close(ev_signal_pair[1]);
	ev_signal_pair[1] = -1;
	sh_old_max = 0;

	free(sh_old);
}

void esignal::evsignal_cb(int fd, short what, void *arg) {
	static char signals[1];
	ssize_t n;

	n = recv(fd, signals, sizeof(signals), 0);
	if (n == -1)
		exit(1);
}

void esignal::evsignal_handler(int sig) {
	int save_errno = errno;

	evsignal_base->sig->evsigcaught[sig]++;
	evsignal_base->sig->evsignal_caught = 1;

	signal(sig, esignal::evsignal_handler);

	send(evsignal_base->sig->ev_signal_pair[0], "a", 1, 0);
	errno = save_errno;
}

int esignal::_evsignal_set_handler(int evsignal,void (*handler)(int))
{

	ev_sighandler_t sh;
	void *p;

	if (evsignal >=sh_old_max) {
			int new_max = evsignal + 1;

			p = realloc(sh_old, new_max * sizeof(*sh_old));
			if (p == NULL) {
				return -1;
			}

			memset((char *)p + sh_old_max * sizeof(*sh_old),
			    0, (new_max - sh_old_max) * sizeof(*sh_old));

			sh_old_max = new_max;
			sh_old = (ev_sighandler_t **)p;
		}

	sh_old[evsignal] = (ev_sighandler_t *)malloc(sizeof *sh_old[evsignal]);
	if (sh_old[evsignal] == NULL) {
		return -1;
	}



	if ((sh = signal(evsignal, handler)) == SIG_ERR) {
		free(sh_old[evsignal]);
		return -1;
	}
	*sh_old[evsignal] = sh;


	return 0;
}

void esignal::evsignal_process() {
	event *ev, *next_ev;
	sig_atomic_t ncalls;
	int i;

	evsignal_caught = 0;
	for (i = 1; i < NSIG; ++i) {
		ncalls = evsigcaught[i];
		if (ncalls == 0)
			continue;
		evsigcaught[i] -= ncalls;

		for (ev = TAILQ_FIRST(&evsigevents[i]); ev != NULL; ev = next_ev) {
			next_ev = TAILQ_NEXT(ev, ev_signal_next);
			if (!(ev->ev_events & EV_PERSIST))
				ev_base->event_del(ev);
			ev_base->event_active(ev, EV_SIGNAL, ncalls);
		}

	}
}
int esignal::evsignal_add(event *ev) {
	int evsignal;

	if (ev->ev_events & (EV_READ | EV_WRITE))
		exit(1);
	evsignal = ev->ev_fd;

	if (TAILQ_EMPTY(&evsigevents[evsignal])) {
		if (_evsignal_set_handler(evsignal,esignal::evsignal_handler) == -1)
			return -1;

		evsignal_base = ev->ev_base;

		if (!ev_signal_added) {
			if (ev_base->event_add(&ev_signal, NULL))
				return -1;
			ev_signal_added = 1;
		}
	}

	TAILQ_INSERT_TAIL(&evsigevents[evsignal], ev, ev_signal_next);

	return 0;
}
int esignal::evsignal_del(event *ev) {
	int evsignal = ev->ev_fd;

	TAILQ_REMOVE(&evsigevents[evsignal], ev, ev_signal_next);

	if (!TAILQ_EMPTY(&evsigevents[evsignal]))
		return 0;


	return (_evsignal_restore_handler(ev->ev_fd));
}

int esignal::_evsignal_restore_handler(int evsignal)
{
	int ret = 0;
	ev_sighandler_t *sh;


	sh = sh_old[evsignal];
	sh_old[evsignal] = NULL;

	if (signal(evsignal, *sh) == SIG_ERR) {
		ret = -1;
	}
	free(sh);

	return ret;
}
