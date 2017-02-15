#pragma once

#include "taskset.h"

class optPFRPAsync
{
private:
	taskset *ts;
	config_info *pconfig;
	int mode; // 0: with sufficient test; 1: no sufficient test
	int search_permissibility_intervals(int k);
	int process_permissibility_intervals(int k);
	int propagate_nodes(int k, INT64 biggestpoint);	
	int task_sched(int k); // create actual scheuding
	int task_simulate(int k);	// simulate scheduling, don't generate actual nodes. -- for the last task

	head_node *list_head;

public:
	INT64 t1; // start of the first perm_interval
	INT64 lmax;

	optPFRPAsync(config_info *_pconfig);
	~optPFRPAsync(void);

	int sched(taskset *ts);
	int setMode(int m);
};

