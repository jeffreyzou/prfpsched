#include "stdafx.h"
#include "statistic.h"
#include <time.h>
#ifdef _THIS_IS_LINUX
#include <sys/time.h>
#else
#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")
#endif

// no runtime data, only data that is constant in every run
//#define TEST_DBG

extern head_node used_list_head; // scheduling1

statistic::statistic(string ss, int n)
{
#ifdef _THIS_IS_LINUX
	string	_logfile = "../PFRPSched/Results/T"+to_string(n)+"/"+ss;
#else
	string	_logfile = ".\\Results\\T"+to_string(n)+"\\"+ss;
#endif
	tag = "T"+to_string(n)+"_"+ss;

	string columns = "tag, status, (lmax status), failed task(deadline), failed job, lcm(n), lcm(n-1), utilization, run_time, ticks, ";
	for(int i=0; i<n; i++)
		columns += "nodes_used[" + to_string(i) + "], ";
	for(int i=0; i<n; i++)
		columns += "nodes_used_after propagation[" + to_string(i) + "], ";
	for(int i=0; i<n; i++)
	{
		columns += "estimated wcrt(lmax)[" + to_string(i) + "], ";
		columns += "real wcrt[" + to_string(i) + "], ";
		columns += "real wcrt[" + to_string(i) + "] at, ";
	}
	columns = columns.substr(0, columns.length() - 1);

	logfile_0 = _logfile + "_sch.csv";
	fout.open(logfile_0);
	fout << columns << endl;
	fout.close();

	logfile_1 = _logfile + "_unsch.csv";
	fout.open(logfile_1);
	fout << columns << endl;
	fout.close();

	logfile_2 = _logfile + "_fmem.csv";
	fout.open(logfile_2);
	fout << columns << endl;
	fout.close();

	logfile_critical = _logfile + "_critical.txt";
	logfile_scheduling = _logfile + "_scheduling.txt";

	num_tried = 0;
	num_sched = 0;	
	num_unsched = 0;	
	num_lmax_failed = 0;
	num_fmem = 0;	
}


statistic::~statistic(void)
{
}

void statistic::start_stat()
{
#ifdef _THIS_IS_LINUX
	gettimeofday(&t_start, NULL);
#else
	time_s = timeGetTime();
#endif	
}

int statistic::end_stat(taskset *ts)
{
#ifdef _THIS_IS_LINUX
	gettimeofday(&t_end, NULL);
	long time_s = ((long)t_start.tv_sec)*1000+(long)t_start.tv_usec/1000;
	long ets = ((long)t_end.tv_sec)*1000+(long)t_end.tv_usec/1000;

#else
	unsigned long ets = timeGetTime();
#endif	
	if(ets < time_s)
		ets += (unsigned long)(-1) - time_s;
	else
		ets -= time_s;

	ts->num_ticks = how_many_ticks(&used_list_head);

	num_tried++;
/*
	if(ts->state == TS_STATE_ERROR)
	{
		string str = "state_error: " + ts->tag;
		write_critical_info(str);
		return ets;
	}
*/
	if(ts->state == TS_STATE_SCHEDED)
		num_sched++;
	else if(ts->state == TS_STATE_UNSCHED)
	{
		if(ts->lmax_fail_at != -1)
			num_lmax_failed++;
		num_unsched++;
	}
	else if(ts->state == TS_STATE_INSUFMEM)
		num_fmem++;

	switch(ts->state)
	{
	case TS_STATE_SCHEDED:
		fout.open(logfile_0, fstream::out | std::fstream::app);
		break;
	case TS_STATE_UNSCHED:
		fout.open(logfile_1, fstream::out | std::fstream::app);
		break;
	case TS_STATE_INSUFMEM:
		fout.open(logfile_2, fstream::out | std::fstream::app);
		break;
	default:
		return ets;
	}
//	string columns = "filename, stat, failedTask, lcmn, lcmnm1, util, run_time, nodes_used, lmaxes, realRespTimes";
	string line;
	line = ts->tag;
	if(ts->state == TS_STATE_SCHEDED)
		line += ", OK";
	else if(TS_STATE_UNSCHED)
		line += ", unschedulable";
	else if(ts->state == TS_STATE_INSUFMEM)
		line += ", mem_failed";
	else
		line += ", undef";

#ifndef TEST_DBG
	if(ts->lmax_fail_at != -1)
		line += ", failed";
	else
#endif
		line += ",";
	if(ts->failed_task != -1)
		line += "," + ts->failedTask+"("+to_string(ts->tq[ts->failed_task]->d)+")";
	else
		line += ",";
	if(ts->wcrt_failed_job != -1)
		line += "," + to_string(ts->wcrt_failed_job);
	else
		line += ",";
#ifdef TEST_DBG
	line += ", ";
	line += ", ";
	line += ", ";
	line += ", ";
	for(int i=0; i<ts->num_task; i++){
		line += ", ";
	}
#else
	line += "," + to_string(ts->LCMs[ts->num_task-1]);
	line += "," + to_string(ts->LCMs[ts->num_task-2]);
	line += "," + to_string(ts->util);
	line += "," + to_string(ets);
	line += "," + to_string(ts->num_ticks);
//	INT64 r = 0;
//	for(int i=0; i<ts->num_task; i++){
//		line += "," + to_string(ts->num_nodes_used[i]-r);
//		r = ts->num_nodes_used[i];
//	}
	for(int i=0; i<ts->num_task; i++){
		line += "," + to_string(ts->num_nodes_used[i]);
	}
	for(int i=0; i<ts->num_task; i++){
		line += "," + to_string(ts->num_nodes_used_after_propagation[i]);
	}
#endif
	for(int i=0; i<ts->num_task; i++)
	{
#ifdef TEST_DBG
		line += ", ";
#else
		line += "," + (ts->estimateWcrt[i]==-1?" ":to_string(ts->estimateWcrt[i]));
#endif
		line += "," + (ts->realWcrt[i]==-1?" ":to_string(ts->realWcrt[i]));
		line += "," + (ts->wcrt_job[i]==-1?" ":to_string(ts->wcrt_job[i]));
	}

	fout << line << endl;
	fout.close();
	return ets;
}

int statistic::write_critical_info(string info)
{
	fout.open(logfile_critical, fstream::out | std::fstream::app);
	fout << info << endl;
	fout.close();
	return 1;
}

int statistic::write_scheduling(string tag, head_node *scheduling_head)
{
	fout.open(logfile_scheduling, fstream::out | std::fstream::app);
	fout <<"+++++++++++++++++++++++++++++" <<tag<< "+++++++++++++++++++++++++++++" << endl;
	fout << "start,end,action,task\n";
	string str1 = "", str2 = "";
	node *p = scheduling_head->first;
	int i=0;
	while(p && i<10000)
	{
		str1 = p->action==0?"normal":"abort";
		str2 = "T"+to_string(p->nTask+1);
		fout << p->start_of_interval << "," << p->end_of_interval << "," << str1 << "," << str2 << endl;
		p = p->next;

		i++;
	}
	fout <<"----------------------------------------------------" <<tag<< "----------------------------------------------------" << endl;
	fout.close();
	return 1;
}

int statistic::write_scheduling_bitmap(string tag, head_node *scheduling_head)
{
	fout.open(logfile_scheduling, fstream::out | std::fstream::app);
	fout <<"+++++++++++++++++++++++++++++" <<tag<< "+++++++++++++++++++++++++++++" << endl;
	node *p = scheduling_head->first;
	int i, j=0;
	for(i=0; i<p->start_of_interval; i++)
		fout << 0;
	while(p && j<20000)
	{
		for(i=0; i<p->end_of_interval-p->start_of_interval; i++)
			fout << 1;
		if(p->next)
		{
			for(i=0; i<p->next->start_of_interval-p->end_of_interval; i++)
				fout << 0;
		}
		p = p->next;

		j++;
	}
	fout <<"----------------------------------------------------" <<tag<< "----------------------------------------------------" << endl;
	fout.close();
	return 1;
}

INT64 statistic:: how_many_ticks(head_node *scheduling_head)
{
	INT64 sum = 0;
//	INT64 temp;
//	INT64 i = 0;
	node *p = scheduling_head->first;
	while(p)
	{
//		temp = sum;
		sum += p->end_of_interval - p->start_of_interval;
		p = p->next;
/*
		i++;
		if(temp >= sum)
		{
			cout << "how_many_ticks(). something wrong." << endl;
		}
	}
*/
	return sum;
}