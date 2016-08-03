#include <iostream>
#include <signal.h>
#include "event.h"
#include "eventbase.h"


int called = 0;
event_base ev_base_test;
static void signal_cb(int fd, short event_, void *arg) {
	event *signal_ = (event *)arg;

	std::cout << "using signal_cb()\r\n";

	if (called >= 2)
		ev_base_test.event_del(signal_);

	called++;
}


int main(int argc, char *argv[]) {
	event ev_test(SIGINT, EV_SIGNAL | EV_PERSIST, signal_cb, &ev_test);
	ev_test.event_base_set(&ev_base_test);
	ev_base_test.event_add(&ev_test, NULL);
	ev_base_test.event_base_loop(0);

	return 0;
}

