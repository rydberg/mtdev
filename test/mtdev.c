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

#include <mtdev-mapping.h>
#include <stdio.h>
#include <fcntl.h>

/* year-proof millisecond event time */
typedef __u64 mstime_t;

static int use_event(const struct input_event *ev)
{
#if 0
	return ev->type == EV_ABS && mtdev_is_absmt(ev->code);
#else
	return 1;
#endif
}

static void print_event(const struct input_event *ev)
{
	static const mstime_t ms = 1000;
	static int slot;
	mstime_t evtime = ev->time.tv_usec / ms + ev->time.tv_sec * ms;
	if (ev->type == EV_ABS && ev->code == ABS_MT_SLOT)
		slot = ev->value;
	fprintf(stderr, "%012llx %02d %01d %04x %d\n",
		evtime, slot, ev->type, ev->code, ev->value);
}

static void loop_device(int fd)
{
	struct mtdev dev;
	struct input_event ev;
	int ret = mtdev_open(&dev, fd);
	if (ret) {
		fprintf(stderr, "error: could not open device: %d\n", ret);
		return;
	}
	while (mtdev_pull(&dev, fd, 1) > 0) {
		while (!mtdev_empty(&dev)) {
			mtdev_get(&dev, &ev);
			if (use_event(&ev))
				print_event(&ev);
		}
	}
	mtdev_close(&dev);
}

int main(int argc, char *argv[])
{
	int fd;
	if (argc < 2) {
		fprintf(stderr, "Usage: mtdev <device>\n");
		return -1;
	}
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "error: could not open device\n");
		return -1;
	}
	if (ioctl(fd, EVIOCGRAB, 1)) {
		fprintf(stderr, "error: could not grab the device\n");
		return -1;
	}
	loop_device(fd);
	ioctl(fd, EVIOCGRAB, 0);
	close(fd);
	return 0;
}
