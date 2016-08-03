#include <stdio.h>
#include "event.h"
#include "eventbase.h"

int lasttime;
event_base ev_base_test;
void timeout_cb(int fd, short event_, void *arg) {
	timeval tv;
	event *timeout = (event*)arg;
	int newtime = time(NULL);

	printf("%s: called at %d: %d\n", __func__, newtime,
	    newtime - lasttime);
	lasttime = newtime;

	evutil_timerclear(&tv);
	tv.tv_sec = 3;
	ev_base_test.event_add(timeout, &tv);
}

int main(int argc, char *argv[]) {
	timeval tv;
	lasttime = time(NULL);
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	event ev_test(-1, 0, timeout_cb, &ev_test);

	ev_test.event_base_set(&ev_base_test);
	ev_base_test.event_add(&ev_test, &tv);

	ev_base_test.event_base_loop(0);

	return 0;
}

