#pragma once

#include "PFRPSched.h"
#include "taskset.h"
#include <fstream>

#ifdef _THIS_IS_LINUX
#include <sys/time.h>
#include <time.h>
#endif



class statistic
{
	string logfile_0;
	string logfile_1;
	string logfile_2;
	string logfile_critical;
	string logfile_scheduling;
	ofstream fout;

public:
	string tag;
#ifdef _THIS_IS_LINUX
	struct timeval t_start, t_end;
#else
	unsigned long time_s;
#endif
	int num_tried;
	int num_sched;
	int num_unsched;
	int num_lmax_failed;
	int num_fmem;

	statistic(string ss, int n);
	~statistic(void);

	void start_stat();
	int end_stat(taskset *ts);
	int write_critical_info(string info);
	int write_scheduling(string tag, head_node *scheduling_head);
	int write_scheduling_bitmap(string tag, head_node *scheduling_head); // 0: idle, 1:busy
	INT64 how_many_ticks(head_node *scheduling_head); 
};
