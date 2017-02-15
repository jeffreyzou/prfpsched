#include "stdafx.h"
#include "taskset.h"

#include <time.h>       /* time */

#ifdef _THIS_IS_LINUX
// for memset()
#include <string.h>
#endif

#include <algorithm>

extern config_info g_config;
extern vector<taskset_t *> g_vec_tasksets;

taskset::taskset(int nt)
{
	num_task = nt;
	LCMs = new INT64[num_task];
	minOffsetsO = new int[num_task];
	minOffsetsZ = new int[num_task];
	memset(minOffsetsZ, 0, num_task*sizeof(int));
	minOffsets = minOffsetsO;
	t_stables = new int[num_task];
	memset(t_stables, 0, num_task*sizeof(int));
	maxOffsets = new int[num_task];
	memset(maxOffsets, 0, num_task*sizeof(int));
	util = 0;
	maxOffset = 0;
	num_nodes_used = new INT64[num_task];
	memset(num_nodes_used, 0, num_task*sizeof(INT64));
	num_nodes_used_after_propagation = new INT64[num_task];
	memset(num_nodes_used_after_propagation, 0, num_task*sizeof(INT64));
	estimateWcrt = new INT64[num_task];
	memset(estimateWcrt, -1, num_task*sizeof(INT64));
	realWcrt = new INT64[num_task];
	memset(realWcrt, -1, num_task*sizeof(INT64));
	wcrt_job = new INT64[num_task];
	memset(wcrt_job, -1, num_task*sizeof(INT64));
	failedTask = "NONE";
	state = TS_STATE_UNSCHED;
	num_ticks = 0;

	failed_task = -1;
	lmax_fail_at = -1;
	wcrt_failed_job = -1;

	t_stable = 0;
}


taskset::~taskset(void)
{
	delete[] estimateWcrt;
	delete[] realWcrt;
	delete[] wcrt_job;
	delete[] num_nodes_used;
	delete[] num_nodes_used_after_propagation;
	delete[] minOffsetsO;
	delete[] minOffsetsZ;
	delete[] LCMs;
	for(int i=0; i<(int)tq.size(); i++)
	{	
		delete tq[i];
	}
}


void taskset::reset(void)
{
	memset(num_nodes_used, 0, num_task*sizeof(INT64));
	memset(num_nodes_used_after_propagation, 0, num_task*sizeof(INT64));
	memset(wcrt_job, -1, num_task*sizeof(INT64));
	memset(realWcrt, -1, num_task*sizeof(INT64));
	memset(estimateWcrt, -1, num_task*sizeof(INT64));
	state = TS_STATE_UNSCHED;
	num_ticks = 0;
	failedTask = "NONE";

	failed_task = -1;
	lmax_fail_at = -1;
	wcrt_failed_job = -1;
}

int taskset::readFromMem(taskset_t *tset)
{
	task *t;
	// use q for random access
	vector<task_t *>::iterator it = tset->vec.begin();
	task_t *tt = *it;
	t = new task(tt->name, tt->c, tt->d, tt->p, tt->offset, tt->pri);
//	t->resp_time = new int[1]; // t0 has only one reponse time: tc
	tq.push_back(t);
	int i = 0;
	LCMs[i] =t->p;
	minOffsets[i] = t->offset;
	minOffset = t->offset;
	maxOffset = t->offset;
	maxOffsets[i] = t->offset;
	t_stables[i] = t->offset;
	t_stable = t_stables[i];
	util = t->util;
	it++;
	i++;
	for(; it!= tset->vec.end(); it++, i++)
	{
		tt = *it;
		t = new task(tt->name, tt->c,tt->d, tt->p, tt->offset,tt->pri);
		LCMs[i] = lcm(LCMs[i-1], t->p);
//		t->resp_time = new int[LCMs[i]/LCMs[i-1] + 3]; // +3 safety zone
		minOffsets[i] = min(minOffsets[i-1], t->offset);
		tq.push_back(t);
		// TODO more research needed
		if(t->offset < maxOffsets[i-1])
		{
			maxOffsets[i] = maxOffsets[i-1];
			t_stables[i] = ceil(maxOffsets[i-1]*1.0/t->p)*t->p+t->offset;
		}
		else
		{
			maxOffsets[i] = t->offset;
			t_stables[i] = t->offset;
		}
		t_stable = max(t_stables[i], t_stable);
		util += t->util;
		maxOffset = max(maxOffset, t->offset);
		minOffset = min(minOffset, t->offset);
	}
	return 0;
}
