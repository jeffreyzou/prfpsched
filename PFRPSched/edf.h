#pragma once

#include "taskset.h"

class edf
{
	taskset *ts;
	config_info *pconfig;
	head_node *list_head;

	INT64 clk;
	int cur_task;
	INT64 biggestpoint;

	int find_next();

public:

	int sched(taskset *ts);

public:
	edf(config_info *_pconfig);
	~edf(void);
};

