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

#ifndef _MTDEV_H
#define _MTDEV_H

#include <linux/input.h>

/* includes available in 2.6.30-rc5 */
#ifndef BTN_TOOL_QUADTAP
#define BTN_TOOL_QUADTAP	0x14f	/* Four fingers on trackpad */
#define ABS_MT_TOUCH_MAJOR	0x30	/* Major axis of touching ellipse */
#define ABS_MT_TOUCH_MINOR	0x31	/* Minor axis (omit if circular) */
#define ABS_MT_WIDTH_MAJOR	0x32	/* Major axis of approaching ellipse */
#define ABS_MT_WIDTH_MINOR	0x33	/* Minor axis (omit if circular) */
#define ABS_MT_ORIENTATION	0x34	/* Ellipse orientation */
#define ABS_MT_POSITION_X	0x35	/* Center X ellipse position */
#define ABS_MT_POSITION_Y	0x36	/* Center Y ellipse position */
#define ABS_MT_TOOL_TYPE	0x37	/* Type of touching device */
#define ABS_MT_BLOB_ID		0x38	/* Group a set of packets as a blob */
#define ABS_MT_TRACKING_ID	0x39	/* Unique ID of initiated contact */
#define SYN_MT_REPORT		2
#define MT_TOOL_FINGER		0
#define MT_TOOL_PEN		1
#endif

/* includes available in 2.6.33 */
#ifndef ABS_MT_PRESSURE
#define ABS_MT_PRESSURE		0x3a	/* Pressure on contact area */
#endif

/* includes available in 2.6.36 */
#ifndef ABS_MT_SLOT
#define ABS_MT_SLOT		0x2f	/* MT slot being modified */
#define MT_SLOT_ABS_EVENTS {	\
	ABS_MT_TOUCH_MAJOR,	\
	ABS_MT_TOUCH_MINOR,	\
	ABS_MT_WIDTH_MAJOR,	\
	ABS_MT_WIDTH_MINOR,	\
	ABS_MT_ORIENTATION,	\
	ABS_MT_POSITION_X,	\
	ABS_MT_POSITION_Y,	\
	ABS_MT_TOOL_TYPE,	\
	ABS_MT_BLOB_ID,		\
	ABS_MT_TRACKING_ID,	\
	ABS_MT_PRESSURE,	\
}
#endif

#define MT_ABS_SIZE 11

/**
 * struct mt_caps - protocol capabilities of kernel device
 * @has_mtdata: true if the device has MT capabilities
 * @has_slot: true if the device sends MT slots
 * @nullid: tracking id used to represent null
 * @slot: slot event properties
 * @abs: ABS_MT event properties
 */
struct mtdev_caps {
	int has_mtdata;
	int has_slot;
	int has_abs[MT_ABS_SIZE];
	int nullid;
	struct input_absinfo slot;
	struct input_absinfo abs[MT_ABS_SIZE];
};

/**
 * struct mtdev - represents an input MT device
 * @caps: the kernel device protocol capabilities
 * @state: internal mtdev parsing state
 *
 * The mtdev structure represents a kernel MT device type B, emitting
 * MT slot events. The events put into mtdev may be from any MT
 * device, specifically type A without contact tracking, type A with
 * contact tracking, or type B with contact tracking. See the kernel
 * documentation for further details.
 *
 */
struct mtdev {
	struct mtdev_caps caps;
	struct mtdev_state *state;
};

/**
 * mtdev_init - initialize mtdev converter
 * @dev: the mtdev to initialize
 *
 * Sets up the internal data structures.
 *
 * Returns zero on success, negative error number otherwise.
 */
int mtdev_init(struct mtdev *dev);

/**
 * mtdev_configure - configure the mtdev converter
 * @dev: the mtdev to configure
 * @fd: file descriptor of the kernel device
 *
 * Reads the device properties to set up the protocol capabilities.
 * If preferred, this can be done by hand, omitting this call.
 *
 * Returns zero on success, negative error number otherwise.
 */
int mtdev_configure(struct mtdev *dev, int fd);

/**
 * mtdev_open - open an mtdev converter
 * @dev: the mtdev to open
 * @fd: file descriptor of the kernel device
 *
 * Initialize the mtdev structure and configure it by reading
 * the protocol capabilities through the file descriptor.
 *
 * Returns zero on success, negative error number otherwise.
 *
 * This call combines mtdev_init() and mtdev_configure(), which
 * may be used separately instead.
 */
int mtdev_open(struct mtdev *dev, int fd);

/**
 * mtdev_idle - check state of kernel device
 * @dev: the mtdev in use
 * @fd: file descriptor of the kernel device
 * @ms: number of milliseconds to wait for activity
 *
 * Returns true if the device is idle, i.e., there are no buffered
 * events and there is nothing to fetch from the kernel device.
 */
int mtdev_idle(struct mtdev *dev, int fd, int ms);

/**
 * mtdev_fetch - fetch an event from the kernel device
 * @dev: the mtdev in use
 * @ev: the kernel input event to fill
 * @fd: file descriptor of the kernel device
 *
 * Fetch a kernel event from the kernel device. The read operation
 * behaves as dictated by the file descriptior; if O_NONBLOCK is not
 * set, the read will block until an event is available.
 *
 * On success, returns the number of events read. Otherwise, a standard
 * negative error number is returned.
 */
int mtdev_fetch(struct mtdev *dev, struct input_event *ev, int fd);

/**
 * mtdev_put - put an event into the converter
 * @dev: the mtdev in use
 * @ev: the kernel input event to put
 *
 * Put a kernel event into the mtdev converter. The event should
 * come straight from the device.
 *
 * This call does not block; if the buffer becomes full, older events
 * are dropped. The buffer is guaranteed to handle several complete MT
 * packets.
 */
void mtdev_put(struct mtdev *dev, const struct input_event *ev);

/**
 * mtdev_pull - pull events from the kernel device
 * @dev: the mtdev in use
 * @fd: file descriptor of the kernel device
 * @max_events: max number of events to read
 *
 * Read a maxmimum of max_events events from the device, and put them
 * in the converter. The read operation behaves as dictated by the
 * file descriptior; if O_NONBLOCK is not set, the read will block
 * until max_events events are available or the buffer is full.
 *
 * On success, returns the number of events read. Otherwise, a standard
 * negative error number is returned.
 *
 * This call combines mtdev_fetch() with mtdev_put(), which
 * may be used separately instead.
 */
int mtdev_pull(struct mtdev *dev, int fd, int max_events);

/**
 * mtdev_empty - check if there are events to get
 * @dev: the mtdev in use
 *
 * Returns true if the event queue is empty, false otherwise.
 */
int mtdev_empty(struct mtdev *dev);

/**
 * mtdev_get - get canonical events from mtdev
 * @dev: the mtdev in use
 * @ev: the input event to fill
 *
 * Get a canonical event from mtdev. The events appear as if they came
 * from a type B device emitting MT slot events.
 *
 * The queue must be non-empty before calling this function.
 */
void mtdev_get(struct mtdev *dev, struct input_event* ev);

/**
 * mtdev_close - close the mtdev converter
 * @dev: the mtdev to close
 *
 * Deallocates all memory associated with mtdev, and sets the state
 * pointer to NULL.
 */
void mtdev_close(struct mtdev *dev);

#endif
