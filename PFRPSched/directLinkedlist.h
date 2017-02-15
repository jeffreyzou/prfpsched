#pragma once

#include "taskset.h"

// sched tasks one by one in the entire schedulability interval
class directLinkedlist
{
private:

	taskset *ts;
	config_info *pconfig;
	int task_sched(int k); // create actual scheuding

	head_node *list_head;

public:
	directLinkedlist(config_info *_pconfig);
	~directLinkedlist(void);

	int sched(taskset *ts);

};

