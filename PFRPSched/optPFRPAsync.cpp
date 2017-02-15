#include "stdafx.h"
#include "optPFRPAsync.h"

#include "PFRPSched.h"
#include "taskset.h"
#include "mmc.h"
#include "statistic.h"

extern mmc *g_mymem;
extern head_node used_list_head; // scheduling
extern statistic *g_stat;

optPFRPAsync::optPFRPAsync(config_info *_pconfig)
{
	pconfig = _pconfig;
	list_head = &used_list_head;
	mode = 0;
}
int optPFRPAsync::setMode(int m)
{
	mode = m;
	return 0;
}

optPFRPAsync::~optPFRPAsync(void)
{
}


int optPFRPAsync::sched(taskset *_ts)
{
	ts = _ts;

	int num_tasks = ts->num_task;
	int i;
	int endstate;
	
	g_stat->start_stat(); // start clocking	

	// a neccessary condition check: no execution time can greater than any deadline(only in sync release?)
	if(pconfig->useoffset == true)
	{
		bool found = false;
		task *t1, *t2, *t=NULL;
		int j, r=0;
		for(i=ts->num_task-1; i>0; i--)
		{
			t1 = ts->tq[i];
			for(j=0; j<i; j++)
			{
				t2 = ts->tq[j];
				if(t1->c >= t2->d)
				{
					t = t1;
					r = i;
				}
				else if(t2->c >= t1->d)
				{
					t = t2;
					r = j;
				}
				else
					continue;
				found = true;
				goto _pre_check_done;
			}
		}
_pre_check_done:
		if(found)
		{
			ts->state = TS_STATE_UNSCHED;
			ts->failedTask = t->name;
			ts->failed_task = r;
			g_stat->end_stat(ts); // stop clocking	
			return ts->state;
		}
	}

	// a simple memory check
	if(pconfig->merge == false)
	{
		INT64 cc = 0;
		for(i=0; i<num_tasks; i++)
			cc += ts->LCMs[num_tasks-2]/ts->tq[i]->p;
		if(g_mymem->num_remains < cc)
		{
			cout << "Too many nodes <needed:remains>: <" << cc << ":" << g_mymem->num_remains << ">" << endl;
			ts->state = TS_STATE_INSUFMEM;
			g_stat->end_stat(ts); // stop clocking	
			return ts->state;
		}
	}

	// sched task 0
	task *t ;
	t = ts->tq[0];
	int tc, toffset;
	tc=t->c, toffset=t->offset;
	attach_node(list_head, NULL, toffset, toffset+tc, pconfig->merge, TS_ACTION_DONE, 0);// no way to fail...
	ts->num_nodes_used[0]++;
	ts->estimateWcrt[0] = tc;
	ts->realWcrt[0] = tc;
	ts->wcrt_job[0] = 0;

	int k; // task k
	endstate = TS_STATE_UNSCHED;
	for(k=1; k<num_tasks-1; k++) // task 0 is scheduled already
	{
		// no sufficient test
		if(mode == 1)
		{
			endstate = task_sched(k); // exact test
			ts->num_nodes_used[k] = list_head->number_of_nodes;
		}
		else
		{
			endstate = process_permissibility_intervals(k); 
			if(endstate == TS_STATE_UNKNOWN)
			{
				endstate = task_sched(k); // exact test
				ts->num_nodes_used[k] = list_head->number_of_nodes;
			}
		}
		if(endstate != TS_STATE_SCHEDED)// cease scheduling
			break;
	}

	// last task
	if(k == num_tasks-1)
	{
		// no sufficient test
		if(mode == 1)
		{
			//endstate = task_sched(k); // exact test
			endstate = task_simulate(k); // exact test
			ts->num_nodes_used[k] = list_head->number_of_nodes;
		}
		else
		{
			endstate = process_permissibility_intervals(k); 
			if(endstate == TS_STATE_UNKNOWN)
			{
				//endstate = task_sched(k); // exact test
				endstate = task_simulate(k); // exact test
				ts->num_nodes_used[k] = list_head->number_of_nodes;
			}
		}
	}

	if(endstate != TS_STATE_SCHEDED)
	{
		ts->failed_task = k;
		ts->failedTask = ts->tq[k]->name;
	}

	ts->state = endstate;
	g_stat->end_stat(ts); // stop clocking
	
	return endstate;
}

int optPFRPAsync::process_permissibility_intervals(int k)
{
	task *t ;
	int tc, td, tp, toffset;
	t = ts->tq[k];
	tc=t->c, td=t->d, tp=t->p, toffset=t->offset;

	int jk = search_permissibility_intervals(k);
	ts->estimateWcrt[k] = lmax;
	if(jk == -1)
	{
		cout << "process_permissibility_intervals, ("<<k<<"), if(jk == -1): " << t->name << endl;
		return TS_STATE_INSUFMEM;
	}
	if(jk == 0)
	{
		cout << "process_permissibility_intervals, ("<<k<<"), if(jk == 0): " << t->name << endl;
//		return TS_STATE_UNSCHED;
		return TS_STATE_UNKNOWN;
	}
	if(t1-toffset+tc > ts->LCMs[k-1])
	{
		if(min(td, tp) < lmax)
		{
			cout << "process_permissibility_intervals, ("<<k<<"), if(min(td, tp) < lmax): " << t->name << endl;
			//goto _end;
			ts->lmax_fail_at = k;
		}
	}
	else
	{
		if(!((td >= lmax) && (tp >= lmax || tp == ts->LCMs[k-1])))
		{
			cout << "process_permissibility_intervals, ("<<k<<"), if(!((td >= lmax) && (tp >= lmax || tp == LCMs[k-1]))): " << t->name << endl;
			//goto _end;
			ts->lmax_fail_at = k;
		}
	}
	return TS_STATE_UNKNOWN;
}

// search in [minOffset(k-1), minOffset(k-1)+lcm(k-1)]
int optPFRPAsync::search_permissibility_intervals(int k)
{
//	cout << k << ": ";

	// step 1
	task *t = ts->tq[k];
	int tc, toffset;
	tc=t->c, toffset=t->offset;
	INT64 biggestpoint = ts->minOffsets[k-1]+ts->LCMs[k-1];

	INT64 temp_lmax = 0; // the longest distance of two consecutive permissibility intervals
	INT64 perm_e = 0; // the END of the LATEST perm interval
	t1 = -1; // the start of the first perm interval
	int jk = 0;
	node *p, *next; // next = p->next
	p = list_head->first; // p always !NULL
	
	INT64 perm_t;
	perm_t = min(p->start_of_interval, biggestpoint);
	// [0, first used)
	if(toffset+tc <= perm_t)
	{
		t1 = toffset;
		perm_e = perm_t;
		jk++;
	}

	next = p->next;
	while(next != NULL)
	{
		if(p->end_of_interval >= biggestpoint)
			break;
		perm_t = min(next->start_of_interval, biggestpoint);
		if(perm_t - p->end_of_interval >= tc)
		{
			if(jk == 0)
				t1 = p->end_of_interval;
			else
				temp_lmax = max(temp_lmax, (p->end_of_interval-perm_e));
			perm_e = perm_t;
			jk++;
		}
		p = next;
		next = p->next;
	}
	// p is the last node
	if((biggestpoint-p->end_of_interval) >= tc)
	{
			if(jk == 0)
				t1 = p->end_of_interval;
			else
				temp_lmax = max(temp_lmax, (biggestpoint-perm_e));
			perm_e = biggestpoint;
			jk++;
	}	

	//step 2.
	if(jk > 0)
	{
		INT64 t2jk; // the end of the last perm interval
		t2jk = perm_e; // zou_20150407
		lmax = max(t1+ts->LCMs[k-1]-t2jk+2*tc-1, t1-toffset+tc);
		if(jk > 1)
			lmax = max(lmax, temp_lmax+2*tc-1);// zou_20150407
	}
	else
		lmax = 0;

	return jk;
}

// propagate nodes from start (minoffset[k-1] or maxoffset[k-1]) to start+lcm[k-1]
int optPFRPAsync::propagate_nodes(int k, INT64 biggestpoint)
{
	// existing schedule
	INT64 start_t = ts->minOffsets[k-1];
	INT64 end_t = start_t + ts->LCMs[k-1];
	INT64 gapO = ts->LCMs[k-1];
	INT64 gap = gapO;
	int state;

	// find the first and last nodes to be copied, they might be an partial, thus have a start offset and end offset.
	node *first, *last, *p, *pnext;
	INT64 soff;
	// until long enough
	INT64 s, e;
	bool bmerge = pconfig->merge;

	// schedule endpoint for the new task
//	INT64 biggestpoint =  ts->minOffsets[k] + ts->LCMs[k];	
	if(end_t >= biggestpoint) // no copy needed
	{
		state = TS_STATE_SCHEDED;
		goto _exit;
	}

	// find the first and last nodes to be copied, they might be an partial, thus have a start offset and end offset.
//	node *first, *last, *p, *pnext;
//	INT64 soff;
	first = NULL;
	last = list_head->last;
	p = list_head->first; // p != null guaranteed 
	while(p)
	{
		soff = p->start_of_interval;
		if(soff >= start_t)
			break;
		// else if(p->start_of_interval < start_t)
		if(p->end_of_interval >= start_t )
		{
			soff = start_t;
			break;
		}
		p = p->next;
	}
	first = p;
	if(!first)
	{
		state = TS_STATE_SCHEDED;
		goto _exit;
	}

	// until long enough
//	INT64 s, e;
//	bool bmerge = pconfig->merge;
	while (list_head->last->end_of_interval < biggestpoint) // p is the cursor runs between *first and *last
	{
		if(p == first) // first one, might be partial
			s = soff + gap;
		else
			s = p->start_of_interval + gap;
		if(s >= biggestpoint)
			break;
		// NOTE the last  node could be merge with the next first node
		if(p == last)
		{
			e = min(p->end_of_interval, end_t) + gap;// last one might be partial

			pnext = first;
			gap += gapO;
		}
		else
		{
			e = p->end_of_interval + gap;

			pnext = p->next;
		}

		if(attach_node(list_head, list_head->last, s, e, bmerge, p->action, p->nTask) < 0) // add to tail
		{
			cout << "ts 1 too many nodes." << list_head->number_of_nodes << ":" << g_mymem->num_remains << endl;
				return TS_STATE_INSUFMEM;
		}
		p = pnext;
	}

	// shrink the last one if it goes out of new_end_t
	if (list_head->last->end_of_interval > biggestpoint)
		list_head->last->end_of_interval = biggestpoint;

	state = TS_STATE_SCHEDED;
_exit:
	ts->num_nodes_used_after_propagation[k-1] = list_head->number_of_nodes;
	return state;
}

int optPFRPAsync::task_sched(int k)
{
	task *t = ts->tq[k];
	int tc, td, tp, toffset;
	tc=t->c, td=t->d, tp=t->p, toffset=t->offset;

	INT64 start_t =  ts->minOffsets[k]; // toffset; // ???? then t migh has not all jobs scheduled
	// schedule endpoint for the new task
	INT64 biggestpoint =start_t + ts->LCMs[k]; 

	// step 1, propagate the used nodes
	if(propagate_nodes(k, biggestpoint) == TS_STATE_INSUFMEM)
		return TS_STATE_INSUFMEM; // insufficient mem

	// check linked list
	if(check_linked_list(list_head) < 0)
	{
		g_stat->write_critical_info(list_head->info);
		return TS_STATE_ERROR;
	}

	bool merge = pconfig->merge;
	bool occupyshort = pconfig->occupyshort;
	int ret = TS_STATE_SCHEDED;

	INT64 wcrt = tc;
	ts->wcrt_job[k] = 0;

	// step 2, simulate tk in [toffset, toffset+lcm[k]]
	INT64 count = ts->LCMs[k]/tp;
	INT64 num_scheded = 0;
	INT64 js; // absolute release time of each job
	js = toffset; // then +tp
	INT64 s, e; // candidate interval

	char action;
	int retval;
	node *p, *pprev;
	pprev = NULL;
	p = list_head->first;
	s = js;
	if(p)
	{
		e = min(s+tc, p->start_of_interval);
		if(s< e) // 1st job of t is scheduled at the head of the list
		{
			if(!occupyshort && e-s < tc) // igore short intervals
				goto _jump_one;

			if(e-s < tc)
				action = TS_ACTION_ABORT ;
			else
				action = TS_ACTION_DONE;
			if(attach_node(list_head, NULL, s, e, merge, action, k) < 0) // add to head
			{
				cout << "ts too many nodes." << list_head->number_of_nodes << ":" << g_mymem->num_remains << endl;
				return TS_STATE_INSUFMEM;
			}
			p = list_head->first;

			wcrt = max(wcrt, e-js); // first one, must be <=td
			if(action == TS_ACTION_DONE)
			{
				ts->wcrt_job[k] = num_scheded;
				num_scheded++;
				js += tp;
			}
		}
		s = max(js, p->end_of_interval);
	}

_jump_one:

	while(num_scheded < count)
	{
		while(true) // roll forward to js
		{
			if(!p) // p is the last used_node, simply sched the rest jobs
			{
				while(num_scheded < count)
				{
					s = (pprev==NULL)?js:max(pprev->end_of_interval, js);
					e = s+tc;
					retval = attach_node(list_head, list_head->last, s, e, merge, TS_ACTION_DONE, k);
					if(retval < 0)
					{
						cout << "ts too many nodes." << list_head->number_of_nodes << ":" << g_mymem->num_remains << endl;
						return TS_STATE_INSUFMEM;
					}
					if(pprev)
					{ // otherwise, response time is always tc
						if(e-js > wcrt)
						{
							wcrt = e-js;
							ts->wcrt_job[k] = num_scheded;
						}
						if(wcrt > td)
							goto _end;
						pprev = NULL;
					}

					num_scheded++;
					js += tp;
					if(js >= biggestpoint)
						break;
				}
				goto _end;
			}

			// update wcrt
			e = min(s+tc, p->start_of_interval);
			if(e<=s || e<=js)
			{
				pprev = p;
				s = max(pprev->end_of_interval, js);
				p = p->next;
				continue;
			}
			if(e-js > wcrt)
			{
				wcrt = e-js;
				ts->wcrt_job[k] = num_scheded;
			}
			if(wcrt > td)
				goto _end;

			if(p->start_of_interval <= js) // keep rolling
			{
				pprev = p;
				s = max(pprev->end_of_interval, js);
				p = p->next;
				continue;
			}
			
			// find a inteval, however might not long enough
			if(!occupyshort && e-s < tc) // igore short intervals, keep looking
			{
				pprev = p;
				s = max(pprev->end_of_interval, js);
				p = p->next;
				continue;
			}

			if(e-s < tc)
				action = TS_ACTION_ABORT;
			else
				action = TS_ACTION_DONE;
			retval = attach_node(list_head, pprev, s, e, merge, action, k);
			if(retval < 0)
			{
				cout << "ts 1 too many nodes." << list_head->number_of_nodes << ":" << g_mymem->num_remains << endl;
				return TS_STATE_INSUFMEM;
			}
//			else if(retval  == 2) // p is merged, don't change pprev
//			else if(retval  == 4) // new node is attached to pprev, don't change pprev
			else if(retval == 3) // new node is inserted to p
				pprev = p;
			else if(retval == 1) // no merge
				pprev = pprev->next;
			s = max(pprev->end_of_interval, js);
			p = pprev->next;

			if(action == TS_ACTION_DONE)
			{
				num_scheded++;
				js += tp;
				s = max(pprev->end_of_interval, js);
				if(js >= biggestpoint)
					goto _end;
				break; // done a job, try next one
			} 
		} // do a job
	} // all jobs

_end:

	ts->realWcrt[k] = wcrt;

	if(num_scheded < count)
	{
		cout << count-num_scheded << " job(s) left when scheduling exits!!!!!!!!!!!!!!!!!!!!!!!\n";
		ts->wcrt_failed_job = num_scheded;
		ret = TS_STATE_UNSCHED;
	}

	// shrink the last one if it goes out of biggestpoint
	p = list_head->last;
	if(p->end_of_interval>biggestpoint)
	{
		if(p->start_of_interval<biggestpoint)
			p->end_of_interval = biggestpoint;
		else
		{
			string str = t->name+": last node is abnormal after sched, [start_of_interval:end_of_interval]= "+to_string(p->start_of_interval)+" : "+to_string(p->end_of_interval);
			cout << str << endl;
			g_stat->write_critical_info(str);
		}
	}

		// check linked list
	if(check_linked_list(list_head) < 0)
	{
		g_stat->write_critical_info(list_head->info);
		return TS_STATE_ERROR;
	}

	return ret;
}

// it simulates scheduling of t_n based on schedue of (n-1) tasks
int optPFRPAsync::task_simulate(int k)
{
	// step 1, propagate the used nodes
	task *t = ts->tq[k];
	int tc, td, tp, toffset;
	tc=t->c, td=t->d, tp=t->p, toffset=t->offset;

	// expand schedule to biggestpoint in case there is a crossborder perm interval
	INT64 biggestpoint =max((INT64)(toffset+tp), ts->minOffsets[k-1] + ts->LCMs[k-1]);
	if(propagate_nodes(k, biggestpoint) == TS_STATE_INSUFMEM)
		return TS_STATE_INSUFMEM; // insufficient mem

	int ret = TS_STATE_SCHEDED;
	INT64 wcrt = 0; // response time

	// step 2, simulate tk in [toffset, toffset+lcm[k])
	//
	INT64 count = ts->LCMs[k]/tp;
	INT64 num_scheded = 0;
	INT64 js; // absolute release time of each job
	INT64 s, e; // candidate interval

	node *p, *pprev;

_start:
	p = list_head->first; // p != NULL
	pprev = NULL;
	js = toffset; // then +tp
	s = js;
	e = min(s+tc, p->start_of_interval); // if e>js+td, it certainly will be sched
	if(s < e) // 1st job of t is scheduled at the head of the list
	{
		if(s+tc <= e)
		{
			wcrt = e-js; //max(wcrt, e-js); // first one, wcrt must = 0 at this moment
			ts->wcrt_job[k] = num_scheded;
			num_scheded++;
			if(num_scheded >= count)
				goto _end;
			js += tp;
			if(js >= biggestpoint) // modular lcm[k-1]
				goto _start;
		}
	}
	s = p->end_of_interval;
	while(num_scheded < count)
	{
		while(true) // roll forward to js
		{
			if(!p) // p is the last used_node, simply sched the rest jobs
			{
				while(num_scheded < count)
				{
					s = (pprev==NULL)?js:max(pprev->end_of_interval, js);
					e = s+tc;
					if(pprev)
					{
						if(e-js > wcrt)
						{
							wcrt = e-js;
							ts->wcrt_job[k] = num_scheded;
						}
						if(wcrt > td)
							goto _end;
						pprev = NULL;
					}

					num_scheded++;
					js += tp;
					if(js >= biggestpoint) // modular lcm[k-1]
						goto _start;
				}
				goto _end;
			}

			// update wcrt
			e = min(s+tc, p->start_of_interval);
			if(e<=s  || e<=js)
			{
				pprev = p;
				s = max(pprev->end_of_interval, js);
				p = p->next;
				continue;
			}
			if(e-js > wcrt)
			{
				wcrt = e-js;
				ts->wcrt_job[k] = num_scheded;
			}
			if(wcrt > td)
				goto _end;

			if(p->start_of_interval <= js) // keep rolling
			{
				pprev = p;
				s = max(pprev->end_of_interval, js);
				p = p->next;
				continue;
			}
			
			// find a inteval, however might not long enough
			if(e-s >= tc)
			{
				if(e-js > wcrt)
				{
					wcrt = e-js;
					ts->wcrt_job[k] = num_scheded;
				}
				if(wcrt > td)
					goto _end;
				num_scheded++;
				js += tp;
				s = max(pprev->end_of_interval, js);

				// expanding schedule takes care of the case that 
				// js<biggest point and this job is schedulable using a crossborder perm_interval--perm[jk+1]
				if(js >= biggestpoint) // modular lcm[k-1]
					goto _start;
				break; // done one job, next one. don't change p, it can be used for next job
			}
			pprev = p;
			s = max(pprev->end_of_interval, js);
			p = p->next;
		}
	}

_end:

	ts->realWcrt[k] = wcrt;

	if(num_scheded<count)
	{
		cout << count - num_scheded << " job(s) left when simulation exits.\n";
		ts->wcrt_failed_job = num_scheded;
		ret = TS_STATE_UNSCHED;
	}

	return ret;
}
