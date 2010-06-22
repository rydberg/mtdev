/*****************************************************************************
 *
 * mtdev - Multitouch Protocol Translation Library (MIT license)
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

#include "state.h"
#include "iobuf.h"
#include "evbuf.h"
#include "match.h"

static inline int istouch(const struct mtdev_slot *data,
			  const struct mtdev_caps *caps)
{
	return data->abs[MTDEV_TOUCH_MAJOR] ||
		!caps->has_abs[MTDEV_TOUCH_MAJOR];
}

/* Dmitry Torokhov's code from kernel/driver/input/input.c */
static int defuzz(int value, int old_val, int fuzz)
{
	if (fuzz) {
		if (value > old_val - fuzz / 2 && value < old_val + fuzz / 2)
			return old_val;

		if (value > old_val - fuzz && value < old_val + fuzz)
			return (old_val * 3 + value) / 4;

		if (value > old_val - fuzz * 2 && value < old_val + fuzz * 2)
			return (old_val + value) / 2;
	}

	return value;
}

/*
 * solve - solve contact matching problem
 * @state: mtdev state
 * @caps: device capabilities
 * @sid: array of current tracking ids
 * @sx: array of current position x
 * @sy: array of current position y
 * @sn: number of current contacts
 * @nid: array of new or matched tracking ids, to be filled
 * @nx: array of new position x
 * @ny: array of new position y
 * @nn: number of new contacts
 * @touch: which of the new contacts to fill
 */
static void solve(struct mtdev_state *state, const struct mtdev_caps *caps,
		  const int *sid, const int *sx, const int *sy, int sn,
		  int *nid, const int *nx, const int *ny, int nn,
		  bitmask_t touch)
{
	int A[DIM2_FINGER], *row;
	int n2s[DIM_FINGER];
	int id, i, j;

	/* setup distance matrix for contact matching */
	for (j = 0; j < sn; j++) {
		row = A + nn * j;
		for (i = 0; i < nn; i++)
			row[i] = dist2(nx[i] - sx[j], ny[i] - sy[j]);
	}

	mtdev_match(n2s, A, nn, sn);

	/* update matched contacts and create new ones */
	foreach_bit(i, touch) {
		j = n2s[i];
		id = j >= 0 ? sid[j] : MT_ID_NULL;
		if (id == MT_ID_NULL)
			id = state->lastid++ & MT_ID_MAX;
		nid[i] = id;
	}
}

/*
 * assign_tracking_id - assign tracking ids to all contacts
 * @state: mtdev state
 * @caps: device capabilities
 * @data: array of all present contacts, to be filled
 * @prop: array of all set contacts properties
 * @size: number of contacts in array
 * @touch: which of the contacts are actual touches
 */
static void assign_tracking_id(struct mtdev_state *state,
			       const struct mtdev_caps *caps,
			       struct mtdev_slot *data, bitmask_t *prop,
			       int size, bitmask_t touch)
{
	int sid[DIM_FINGER], sx[DIM_FINGER], sy[DIM_FINGER], sn = 0;
	int nid[DIM_FINGER], nx[DIM_FINGER], ny[DIM_FINGER], i;
	foreach_bit(i, state->used) {
		sid[sn] = state->data[i].abs[MTDEV_TRACKING_ID];
		sx[sn] = state->data[i].abs[MTDEV_POSITION_X];
		sy[sn] = state->data[i].abs[MTDEV_POSITION_Y];
		sn++;
	}
	for (i = 0; i < size; i++) {
		nx[i] = data[i].abs[MTDEV_POSITION_X];
		ny[i] = data[i].abs[MTDEV_POSITION_Y];
	}
	solve(state, caps, sid, sx, sy, sn, nid, nx, ny, size, touch);
	for (i = 0; i < size; i++) {
		data[i].abs[MTDEV_TRACKING_ID] =
			GETBIT(touch, i) ? nid[i] : MT_ID_NULL;
		prop[i] |= BITMASK(MTDEV_TRACKING_ID);
	}
}

/*
 * process_typeA - consume MT events and update mtdev state
 * @state: mtdev state
 * @data: array of all present contacts, to be filled
 * @prop: array of all set contacts properties, to be filled
 *
 * This function is called when a SYN_REPORT is seen, right before
 * that event is pushed to the queue.
 *
 * Returns -1 if the packet is not MT related and should not affect
 * the current mtdev state.
 */
static int process_typeA(struct mtdev_state *state,
			 struct mtdev_slot *data, bitmask_t *prop)
{
	struct input_event ev;
	int consumed, mtcode;
	int mtcnt = 0, size = 0;
	prop[size] = 0;
	while (!evbuf_empty(&state->inbuf)) {
		evbuf_get(&state->inbuf, &ev);
		consumed = 0;
		switch (ev.type) {
		case EV_SYN:
			switch (ev.code) {
			case SYN_MT_REPORT:
				if (size < DIM_FINGER &&
				    GETBIT(prop[size], MTDEV_POSITION_X) &&
				    GETBIT(prop[size], MTDEV_POSITION_Y))
					size++;
				if (size < DIM_FINGER)
					prop[size] = 0;
				mtcnt++;
				consumed = 1;
				break;
			}
			break;
		case EV_KEY:
			switch (ev.code) {
			case BTN_TOUCH:
				mtcnt++;
				break;
			}
			break;
		case EV_ABS:
			if (size < DIM_FINGER && mtdev_is_absmt(ev.code)) {
				mtcode = mtdev_abs2mt(ev.code);
				data[size].abs[mtcode] = ev.value;
				prop[size] |= BITMASK(mtcode);
				mtcnt++;
				consumed = 1;
			}
			break;
		}
		if (!consumed)
			evbuf_put(&state->outbuf, &ev);
	}
	return mtcnt ? size : -1;
}

/*
 * process_typeB - propagate events without parsing
 * @state: mtdev state
 *
 * This function is called when a SYN_REPORT is seen, right before
 * that event is pushed to the queue.
 */
static void process_typeB(struct mtdev_state *state)
{
	struct input_event ev;
	while (!evbuf_empty(&state->inbuf)) {
		evbuf_get(&state->inbuf, &ev);
		evbuf_put(&state->outbuf, &ev);
	}
}

/*
 * filter_data - apply input filtering on new incoming data
 * @state: mtdev state
 * @caps: device capabilities
 * @data: the incoming data to filter
 * @prop: the properties to filter
 * @slot: the slot the data refers to
 */
static void filter_data(const struct mtdev_state *state,
			const struct mtdev_caps *caps,
			struct mtdev_slot *data, bitmask_t prop,
			int slot)
{
	int i;
	foreach_bit(i, prop) {
		int fuzz = caps->abs[i].fuzz;
		int oldval = state->data[slot].abs[i];
		data->abs[i] = defuzz(data->abs[i], oldval, fuzz);
	}
}

/*
 * push_slot_changes - propagate state changes
 * @state: mtdev state
 * @data: the incoming data to propagate
 * @prop: the properties to propagate
 * @slot: the slot the data refers to
 * @syn: reference to the SYN_REPORT event
 */
static void push_slot_changes(struct mtdev_state *state,
			      const struct mtdev_slot *data, bitmask_t prop,
			      int slot, const struct input_event *syn)
{
	struct input_event ev;
	int i, count = 0;
	foreach_bit(i, prop)
		if (state->data[slot].abs[i] != data->abs[i])
			count++;
	if (!count)
		return;
	ev.time = syn->time;
	ev.type = EV_ABS;
	ev.code = ABS_MT_SLOT;
	ev.value = slot;
	if (state->slot != ev.value) {
		evbuf_put(&state->outbuf, &ev);
		state->slot = ev.value;
	}
	foreach_bit(i, prop) {
		ev.code = mtdev_mt2abs(i);
		ev.value = data->abs[i];
		if (state->data[slot].abs[i] != ev.value) {
			evbuf_put(&state->outbuf, &ev);
			state->data[slot].abs[i] = ev.value;
		}
	}
}

/*
 * apply_typeA_changes - parse and propagate state changes
 * @state: mtdev state
 * @caps: device capabilities
 * @data: array of data to apply
 * @prop: array of properties to apply
 * @size: number of contacts in array
 * @syn: reference to the SYN_REPORT event
 */
static void apply_typeA_changes(struct mtdev_state *state,
				const struct mtdev_caps *caps,
				struct mtdev_slot *data, const bitmask_t *prop,
				int size, const struct input_event *syn)
{
	bitmask_t unused = ~state->used;
	bitmask_t used = 0;
	int i, slot, id;
	for (i = 0; i < size; i++) {
		id = data[i].abs[MTDEV_TRACKING_ID];
		foreach_bit(slot, state->used) {
			if (state->data[slot].abs[MTDEV_TRACKING_ID] != id)
				continue;
			filter_data(state, caps, &data[i], prop[i], slot);
			push_slot_changes(state, &data[i], prop[i], slot, syn);
			SETBIT(used, slot);
			id = MT_ID_NULL;
			break;
		}
		if (id != MT_ID_NULL) {
			slot = firstbit(unused);
			push_slot_changes(state, &data[i], prop[i], slot, syn);
			SETBIT(used, slot);
			CLEARBIT(unused, slot);
		}
	}

	/* clear unused slots and update slot usage */
	foreach_bit(slot, state->used & ~used) {
		struct mtdev_slot tdata = state->data[slot];
		bitmask_t tprop = BITMASK(MTDEV_TRACKING_ID);
		tdata.abs[MTDEV_TRACKING_ID] = MT_ID_NULL;
		push_slot_changes(state, &tdata, tprop, slot, syn);
	}
	state->used = used;
}

/*
 * convert_A_to_B - propagate a type A packet as a type B packet
 * @state: mtdev state
 * @caps: device capabilities
 * @syn: reference to the SYN_REPORT event
 */
static void convert_A_to_B(struct mtdev_state *state,
			   const struct mtdev_caps *caps,
			   const struct input_event *syn)
{
	struct mtdev_slot data[DIM_FINGER];
	bitmask_t prop[DIM_FINGER];
	int size = process_typeA(state, data, prop);
	if (size < 0)
		return;
	if (!caps->has_abs[MTDEV_TRACKING_ID]) {
		bitmask_t touch = 0;
		int i;
		for (i = 0; i < size; i++)
			MODBIT(touch, i, istouch(&data[i], caps));
		assign_tracking_id(state, caps, data, prop, size, touch);
	}
	apply_typeA_changes(state, caps, data, prop, size, syn);
}

int mtdev_init(struct mtdev *dev)
{
	int i;
	memset(dev, 0, sizeof(struct mtdev));
	dev->state = calloc(1, sizeof(struct mtdev_state));
	if (!dev->state)
		return -ENOMEM;
	for (i = 0; i < DIM_FINGER; i++)
		dev->state->data[i].abs[MTDEV_TRACKING_ID] = MT_ID_NULL;
	return 0;
}

int mtdev_open(struct mtdev *dev, int fd)
{
	int ret;
	ret = mtdev_init(dev);
	if (ret)
		goto error;
	ret = mtdev_configure(dev, fd);
	if (ret)
		goto mtdev;
	return 0;
 mtdev:
	mtdev_close(dev);
 error:
	return ret;
}

void mtdev_put_event(struct mtdev *dev, const struct input_event *ev)
{
	struct mtdev_state *state = dev->state;
	if (ev->type == EV_SYN && ev->code == SYN_REPORT) {
		bitmask_t head = state->outbuf.head;
		if (dev->state)
			convert_A_to_B(state, &dev->caps, ev);
		else
			process_typeB(state);
		if (state->outbuf.head != head)
			evbuf_put(&state->outbuf, ev);
	} else {
		evbuf_put(&state->inbuf, ev);
	}
}

void mtdev_close(struct mtdev *dev)
{
	free(dev->state);
	memset(dev, 0, sizeof(struct mtdev));
}
