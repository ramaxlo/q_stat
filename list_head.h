/*
 * list_head.h - utilities for handling doubly linked list
 *
 * Author: Ramax Lo <ramax.lo@vivotek.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef _LIST_HEAD_H_
#define _LIST_HEAD_H_

struct TListHead
{
	struct TListHead *ptPrev;
	struct TListHead *ptNext;
};

#define container_of(ptr, st, en) \
	({char *p = (char *)&(((st *)0)->en); (st *)((char *)ptr - (unsigned long)p);})

#define foreach(e, list) \
	for (e = (list)->ptNext; e != (list); e = e->ptNext)

#define foreach_safe(e, tmp, list) \
	for (e = (list)->ptNext, tmp = e->ptNext; e != (list); e = tmp, tmp = tmp->ptNext)

#define LIST_INIT(x) \
	struct TListHead x = {&x, &x}

static inline void InitList(struct TListHead *ptEntry)
{
	ptEntry->ptNext = ptEntry;
	ptEntry->ptPrev = ptEntry;
}

static inline struct TListHead *ListNext(struct TListHead *ptEntry)
{
	return ptEntry->ptNext;
}

static inline struct TListHead *ListPrev(struct TListHead *ptEntry)
{
	return ptEntry->ptPrev;
}

static inline int ListIsEmpty(struct TListHead *ptEntry)
{
	return (ptEntry->ptNext != ptEntry) ? 0 : 1;
}

static inline void ListAdd(struct TListHead *ptEntry, struct TListHead *ptList)
{
	ptEntry->ptNext = ptList->ptNext;
	ptEntry->ptPrev = ptList;
	ptList->ptNext->ptPrev = ptEntry;
	ptList->ptNext = ptEntry;
}

static inline void ListRemove(struct TListHead *ptEntry)
{
	ptEntry->ptPrev->ptNext = ptEntry->ptNext;
	ptEntry->ptNext->ptPrev = ptEntry->ptPrev;
	ptEntry->ptNext = NULL;
	ptEntry->ptPrev = NULL;
}

static inline void ListAddTail(struct TListHead *ptEntry, struct TListHead *ptList)
{
	ListAdd(ptEntry, ptList->ptPrev);
}

static inline void ListAddHead(struct TListHead *ptEntry, struct TListHead *ptList)
{
	ListAdd(ptEntry, ptList);
}

#endif // _LIST_HEAD_H_

