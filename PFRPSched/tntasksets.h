#pragma once

#include "PFRPSched.h"

class tntasksets
{
private:
	task_t * parseLine(char *line);
	vector<taskset_t *> *_vec_tasksets;
public:
	tntasksets(void);
	~tntasksets(void);

	string filename;
	int lmax_failed_task_num;
	int wcrt_failed_task_num;
	int scheded_num;
	int sched_failed_num;
	int mem_failed_num;
	int readTasksetsFromFile(string filepath, vector<taskset_t *>  *vec_tasksets);

};

