/*****************************************************************************
 *
 * mtdev - MT device event converter (MIT license)
 *
 * Copyright (C) 2010 Henrik Rydberg <rydberg@euromail.se>
 * Copyright (C) 2010 Canonical Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************/

#include "iobuf.h"
#include "state.h"
#include <poll.h>

int mtdev_idle(struct mtdev *dev, int fd, int ms)
{
	struct mtdev_iobuf *buf = &dev->state->iobuf;
        struct pollfd fds = { fd, POLLIN, 0 };
	return	buf->head == buf->tail && poll(&fds, 1, ms) <= 0;
}

int mtdev_fetch(struct mtdev *dev, int fd, struct input_event *ev)
{
	struct mtdev_iobuf *buf = &dev->state->iobuf;
	int n = buf->head - buf->tail;
	if (n < EVENT_SIZE) {
		if (buf->tail && n > 0)
			memmove(buf->data, buf->data + buf->tail, n);
		buf->head = n;
		buf->tail = 0;
		SYSCALL(n = read(fd, buf->data + buf->head,
				 DIM_BUFFER - buf->head));
		if (n <= 0)
			return n;
		buf->head += n;
	}
	if (buf->head - buf->tail < EVENT_SIZE)
		return 0;
	memcpy(ev, buf->data + buf->tail, EVENT_SIZE);
	buf->tail += EVENT_SIZE;
	return 1;
}

int mtdev_pull(struct mtdev *dev, int fd, int max_events)
{
	struct mtdev_state *state = dev->state;
	struct input_event ev;
	int ret, count = 0;
	if (max_events <= 0)
		max_events = DIM_EVENTS;
	while (max_events-- && !evbuf_full(&state->inbuf)) {
		ret = mtdev_fetch(dev, fd, &ev);
		if (ret < 0)
			return ret;
		if (ret == 0)
			break;
		mtdev_put(dev, &ev);
		count++;
	}
	return count;
}
