#include "stdafx.h"
#include "directLinkedlist.h"

#include "mmc.h"
#include "statistic.h"

extern mmc *g_mymem;
extern head_node used_list_head; // scheduling
extern statistic *g_stat;

directLinkedlist::directLinkedlist(config_info *_pconfig)
{
	pconfig = _pconfig;
	list_head = &used_list_head;
}

directLinkedlist::~directLinkedlist(void)
{
}


int directLinkedlist::sched(taskset *_ts)
{
	ts = _ts;

	int num_tasks = ts->num_task;
	int i;
	int endstate = TS_STATE_UNSCHED;
	
	g_stat->start_stat(); // start clocking	

	// a neccessary condition check: no execution time can greater than any deadline(only in sync release?)
	if(pconfig->useoffset == true)
	{
		bool found = false;
		task *t1, *t2, *t = NULL;
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
			cc += 2*ts->LCMs[num_tasks-1]/ts->tq[i]->p;
		if(g_mymem->num_remains < cc)
		{
			cout << "Too many nodes <needed:remains>: <" << cc << ":" << g_mymem->num_remains << ">" << endl;
			ts->state = TS_STATE_INSUFMEM;
			g_stat->end_stat(ts); // stop clocking	
			return ts->state;
		}
	}

	int k;
	for(k=0; k<ts->num_task; k++)
	{
		endstate = task_sched(k);
		ts->num_nodes_used[k] = list_head->number_of_nodes;
		if(endstate != TS_STATE_SCHEDED)
		{
			ts->failed_task = k;
			ts->failedTask = ts->tq[k]->name;
			break;
		}
	}

	ts->state = endstate;
	g_stat->end_stat(ts); // stop clocking
	
	return endstate;
}

// sched interval: [toffset, t_stable+LCM)
// periodic sched: [t_stable, t_stable+LCM)
int directLinkedlist::task_sched(int k)
{
	task *t = ts->tq[k];
	int tc, td, tp, toffset;
	tc=t->c, td=t->d, tp=t->p, toffset=t->offset;

	INT64 start_t =  toffset;
	// schedule endpoint for the new task
	INT64 biggestpoint =ts->t_stable + ts->LCMs[ts->num_task-1];

	bool merge = pconfig->merge;
	bool occupyshort = pconfig->occupyshort;
	int ret = TS_STATE_SCHEDED;

	INT64 wcrt = tc;
	ts->wcrt_job[k] = 0;

	// [toffset, t_stable+LCM)
	INT64 count = (biggestpoint-1-start_t)/tp+1; // -1 means right open interval, +1 is the first job at start_t
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
			string str =  t->name+": last node is abnormal after sched, [start_of_interval:end_of_interval]= "+to_string(p->start_of_interval)+" : "+to_string(p->end_of_interval);
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
