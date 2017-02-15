#include "stdafx.h"
#include "PFRPSched.h"
#include "mmc.h"

#include <fstream>

extern mmc *g_mymem;

INT64 gcd(INT64 a, INT64 b)
{
  if (b == 0)
    return a;
  else
    return gcd(b, a%b);
}

INT64 lcm(INT64 a, INT64 b)
{
  return (a*b)/gcd(a,b);
}

// insert a node to the head of a non-empty list
int insert_first_node(head_node *l, INT64 s, INT64 e, bool merge, char action, char nTask)
{
	node *first = l->first;
	if(merge)
	{
		if(first->start_of_interval <= e-s) // joint
		{
			first->start_of_interval = 0;
			return 2;
		}
		if(first->start_of_interval == e) // insert
		{
			first->start_of_interval = s;
			return 3;
		}
	}
	// don't merge or not eligible for merge
	node *q = g_mymem->alloc_node();
	if(!q)
		return -1;
	q->start_of_interval = s;
	q->end_of_interval = e;
	q->next = NULL;
	q->action = action;	
	q->nTask = nTask;

	q->next = first;
	l->first = q;
	l->number_of_nodes++;

	return 1;
}

// merge node in the single-ordered-list: <s is guaranteed to >= p->end>
// (1) p->next->start - p->end <= len ------ extend p, delete p->next
// (2) else if p->end == s, extend p
// (3) else if p->next->start = e, extend p->next
// (4) don't merge or not eligible above 3 conditions

// p == null means insert as l->first to a non-empty list
// p->next might be delete because of merge
// however, p is valid whatever
// return:
//	 0 - nothing change for empty node or no list exist
//	-1 - memory insufficient
//	 1 - ok
int attach_node(head_node *l, node *p, INT64 s, INT64 e, bool merge, char action, char nTask)
{
	if(e <= s)
		return 0;
	if(l == NULL)
		return 0;
	
	if(l->first == NULL) // ignore p
	{ 
		node *q = g_mymem->alloc_node();
		if(!q)
			return -1;
		q->start_of_interval = s;
		q->end_of_interval = e;
		q->next = NULL;
		q->action = action;
		q->nTask = nTask;

		l->first = q;
		l->last = q;
		l->number_of_nodes++;
		return 1;
	}

	// insert to head of the list
	if(p == NULL)
		return insert_first_node(l, s, e, merge, action, nTask);

	// merge adjascent nodes
	if(merge)
	{
		node *next = p->next; // may need to merge p->next too
		if(next)
		{
			if(next->start_of_interval - p->end_of_interval <= e - s) // joint
			{
				p->end_of_interval = next->end_of_interval;
				p->next = next->next;
				if(l->last == next)
					l->last = p;
				l->number_of_nodes--;
				g_mymem->free_node(next);
				return 2;
			}
			if(next->start_of_interval == e) // insert to next
			{
				next->start_of_interval = s;
				return 3;
			}
		}
		if(p->end_of_interval == s) // attach to current
		{
			p->end_of_interval = e; // just extend its end
			return 4;
		}
	}

	// don't merge or not eligible for merge
	node *q = g_mymem->alloc_node();
	if(!q)
		return -1;
	q->start_of_interval = s;
	q->end_of_interval = e;
	q->next = NULL;
	q->action = action;
	q->nTask = nTask;

	q->next = p->next;
	p->next = q;	
	if(l->last == p)
		l->last = q;
	l->number_of_nodes++;

	return 1;
}

int check_linked_list(head_node *list_head)
{
#ifdef MY_DEBUG

	if(list_head->first == NULL)
		return 0;

	node *p, *next;
	p = list_head->first;
	next = p->next;
	while(next)
	{
		// empt node? or even worse?
		if(p->start_of_interval >= p->end_of_interval)
		{
			list_head->info = "illegal node, p->start_of_interval >= p->end_of_interval: "+to_string(p->start_of_interval)+" : "+to_string(p->end_of_interval);
			cout << list_head->info << endl;
			return -1;
		}

		// sorted?
		if(next->start_of_interval < p->end_of_interval)
		{
			list_head->info = "out of order, next->start_of_interval < p->end_of_interval: "+to_string(next->start_of_interval)+" : "+to_string(p->end_of_interval);
			cout << list_head->info << endl;
			return -1;
		}

		p = next;
		next = p->next;
	}
//	if(p) // of course, p is the last
	{
		// empt node? or even worse?
		if(p->start_of_interval >= p->end_of_interval)
		{
			list_head->info = "illegal node, p->start_of_interval >= p->end_of_interval: "+to_string(p->start_of_interval)+" : "+to_string(p->end_of_interval);
			cout << list_head->info << endl;
			return -1;
		}
	}

#endif

	return 0;
}