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

#include "common.h"

#define SETABS(c, x, map, key, fd)					\
	(c->has_##x = getbit(map, key) && getabs(&c->x, key, fd))

static const int SN_COORD = 250;	/* coordinate signal-to-noise ratio */
static const int SN_WIDTH = 100;	/* width signal-to-noise ratio */
static const int SN_ORIENT = 10;	/* orientation signal-to-noise ratio */

static const int bits_per_long = 8 * sizeof(long);

static inline int nlongs(int nbit)
{
	return (nbit + bits_per_long - 1) / bits_per_long;
}

static inline int getbit(const unsigned long *map, int key)
{
	return (map[key / bits_per_long] >> (key % bits_per_long)) & 0x01;
}

static int getabs(struct input_absinfo *abs, int key, int fd)
{
	int rc;
	SYSCALL(rc = ioctl(fd, EVIOCGABS(key), abs));
	return rc >= 0;
}

static int has_mt_data(const struct mtdev_caps *cap)
{
	return cap->has_abs[MTDEV_POSITION_X] && cap->has_abs[MTDEV_POSITION_Y];
}

static void default_fuzz(struct mtdev_caps *cap, int bit, int sn)
{
	if (cap->has_abs[bit] && cap->abs[bit].fuzz == 0)
		cap->abs[bit].fuzz =
			(cap->abs[bit].maximum - cap->abs[bit].minimum) / sn;
}

static int read_caps(struct mtdev_caps *cap, int fd)
{
	unsigned long absbits[nlongs(ABS_MAX)];
	int rc, i;

	memset(cap, 0, sizeof(struct mtdev_caps));

	SYSCALL(rc = ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits));
	if (rc < 0)
		return rc;

	SETABS(cap, slot, absbits, ABS_MT_SLOT, fd);
	for (i = 0; i < MT_ABS_SIZE; i++)
		SETABS(cap, abs[i], absbits, mtdev_mt2abs(i), fd);

	cap->has_mtdata = has_mt_data(cap);

	if (cap->has_abs[MTDEV_TRACKING_ID])
		cap->nullid = cap->abs[MTDEV_TRACKING_ID].minimum - 1;

	default_fuzz(cap, MTDEV_POSITION_X, SN_COORD);
	default_fuzz(cap, MTDEV_POSITION_Y, SN_COORD);
	default_fuzz(cap, MTDEV_TOUCH_MAJOR, SN_WIDTH);
	default_fuzz(cap, MTDEV_TOUCH_MINOR, SN_WIDTH);
	default_fuzz(cap, MTDEV_WIDTH_MAJOR, SN_WIDTH);
	default_fuzz(cap, MTDEV_WIDTH_MINOR, SN_WIDTH);
	default_fuzz(cap, MTDEV_ORIENTATION, SN_ORIENT);

	return 0;
}

int mtdev_configure(struct mtdev *dev, int fd)
{
	return read_caps(&dev->caps, fd);
}
