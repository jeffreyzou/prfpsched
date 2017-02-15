#pragma once

#include "PFRPSched.h"
#include <deque>


class taskset
{
private:

	task_t * parseLine(char *line);

public:
	string tag;
	string failedTask;
	int failed_task;
	INT64 wcrt_failed_job;
	INT64 lmax_fail_at;
	deque<task *> tq;
	INT64 *LCMs;
	int *minOffsetsO;
	int *minOffsetsZ;
	int *minOffsets;
	int minOffset;
	int maxOffset;
	int *maxOffsets;
	int *t_stables; // stable sched points
	int t_stable; // max{t_stables}
	INT64 *num_nodes_used;
	INT64 *num_nodes_used_after_propagation;
	INT64 *estimateWcrt;
	INT64 *realWcrt;
	INT64 *wcrt_job;  // the (first) job which has wcrt
	int num_task;
	double util;

	int state; // end state, 0: scheduled, 1:un_schedulable, 2: insufficient memory, 3: empty ...
	INT64 num_ticks; // how many cpu ticks are used

	taskset(int nt);
	~taskset(void);

	void reset();

	int readFromMem(taskset_t *tset);
};


