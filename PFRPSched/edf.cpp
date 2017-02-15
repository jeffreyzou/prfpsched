#include "stdafx.h"
#include "edf.h"

#include "mmc.h"
#include "statistic.h"

extern mmc *g_mymem;
extern head_node used_list_head; // scheduling
extern statistic *g_stat;


edf::edf(config_info *_pconfig)
{
	pconfig = _pconfig;
	list_head = &used_list_head;
}


edf::~edf(void)
{
}

int edf::sched(taskset *_ts)
{
	ts = _ts;
	clk = 0;
	int num_tasks = ts->num_task;
	biggestpoint =ts->maxOffset + ts->LCMs[ts->num_task-1];
	cur_task = -1;

	task *t;
	int i;
	int endstate = TS_STATE_UNSCHED;;
	// find first
	INT64 finish_t = biggestpoint;
	for(i=0; i<num_tasks; i++)
	{
		t = ts->tq[i];
		if(t->offset < finish_t)
		{
			finish_t = t->offset;
			cur_task = i;
		}
	}

	// find next
		/*
	int next_task = find_next();

	if(next_task == -1)
		e = clk + ts->tq[cur_task]->incomplete_c;
	else
		e = clk + ts->tq[cur_task]->offset + 

				if(e-s < tc)
				action = TS_ACTION_ABORT ;
			else
				action = TS_ACTION_DONE;

				if(attach_node(list_head, NULL, s, e, merge, action, k) < 0) // add to head
			{
				cout << "ts too many nodes." << list_head->number_of_nodes << ":" << g_mymem->num_remains << endl;
				return TS_STATE_INSUFMEM;
			}

*/
	
//	g_stat->start_stat(); // start clocking	

	return endstate;

}

// return:
// -1 -- current task runs to end;
// others

int edf::find_next()
{
	int i;
	int next_task = -1;
	task *t;

	// check if there is a higher priority task release before current task finishes
	INT64 finish_t; 
	INT64 val;
	if(cur_task == -1)
		finish_t = biggestpoint;
	else
		finish_t = clk+ts->tq[cur_task]->c;
	
	for(i=0; i<cur_task; i++)
	{
		t = ts->tq[i];
		val = (clk-t->offset)/t->p + t->p;
		if(val < finish_t)
		{
			finish_t = (clk-t->offset)/t->p + t->p;
			next_task = i;
		}
	}

//	if(next_task == -1) // current task runs to end
	return next_task;
}
