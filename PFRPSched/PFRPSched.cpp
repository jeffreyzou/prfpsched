// PFRPSched.cpp: Defines the entry point for the console application.
//

#include "stdafx.h"
#include "mmc.h"
#include "statistic.h"
#include "tntasksets.h"
#include "PFRPSched.h"

#include "optPFRPAsync.h"
#include "directLinkedlist.h"

#ifndef _THIS_IS_LINUX
#include <io.h>

#define ACCESS _access
#define MKDIR(a) _mkdir((a))
#endif

#include <deque>
#include <array>
#include <vector>
#include <fstream>

head_node used_list_head; // scheduling

string g_taskfile;
statistic *g_stat;
mmc *g_mymem;

config_info g_config;
config_info *g_configs;
vector<taskset_t *> g_vec_tasksets; // the original tasksets for all Tn-task system

string g_srcfolder;
string g_resultfolder;

INT64 lmax, t1; // for optPFRPAsync

// algorithms
optPFRPAsync *g_optPFRPAsync;
optPFRPAsync *g_optPFRPAsyncNoSufficientTest;
directLinkedlist *g_directLinkedlist;

void init();
void leave();
void cleanup();

//int worker(taskset *ts); // work on one taskset

// work on one taskset for all algorithms and configuritions first
// load taskfile only once
void plan_first(int k);
// finish all tasksets of T_N for one algorithm first
// can load taskfiles into g_vec_tasksets[] at the begining
//void plan_second();

#ifdef _THIS_IS_LINUX
int main(int argc, char* argv[])
{
	g_srcfolder	= "../PFRPSched/Files";
	g_resultfolder	= "../PFRPSched/Results";

#else
int _tmain(int argc, char* argv[])
{
	g_srcfolder	= ".\\Files";
	g_resultfolder	= ".\\Results";

	system("rmdir .\\Results /s /q");
	system("xcopy .\\ResultsO .\\Results /e /t /i");

#endif	

	if(argc > 1)
	{
		g_srcfolder = argv[1];
		if(argc >2)
			g_resultfolder = argv[2];
	}

	init();

	for(int i=3; i<11; i++)
		plan_first(i);

	// plan_second();

	leave();
	
#ifdef 	_THIS_IS_LINUX
	cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!DONE!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
#else
	system("pause");
#endif

	return 0;
}
/*
void worker()
{
	_finddata_t fileDir;
    long long lfDir;
	string dir = g_srcfolder + "\\";
	string dir_1 = dir + "*.txt";

	if((lfDir=_findfirst(dir_1.c_str(),&fileDir)) == -1)
	{
        cout << "No file is found in "<< dir << endl;
		return;
	}
	vector<string> vec_taskfiles;
	do{
		vec_taskfiles.push_back(fileDir.name);
		cout << dir + fileDir.name << endl;
	}while( _findnext( (intptr_t)lfDir, &fileDir ) == 0 );
    _findclose((intptr_t)lfDir);


}
*/
void plan_first(int k)
{
	// alg-merge-offset-occupyshort
	// alg: 0-optPFRP, 1-directlinkedlist, 2-optPFRPNoSufficientTest
	enum configs_enum_t {oFFF, oFFT, oFTF, oFTT, oTFF, oTFT, oTTF, oTTT,
										 iFFF, iFFT, iFTF, iFTT, iTFF, iTFT, iTTF, iTTT, 
										 iiFFF, iiFFT, iiFTF, iiFTT, iiTFF, iiTFT, iiTTF, iiTTT
										};
#define NUM_DO_CONFIGS	4
	int doConfigs[NUM_DO_CONFIGS] = {iiTFF, iiTTF, iiTFT, iTFF};

	g_vec_tasksets.clear();
	tntasksets *tnts = new tntasksets();
	string ss;
#ifdef _THIS_IS_LINUX
	ss = "../PFRPSched/Files/T"+to_string(k)+".txt";
#else
	ss = ".\\Files\\T"+to_string(k)+".txt";
#endif
	tnts->readTasksetsFromFile(ss, &g_vec_tasksets);
	int nt = g_vec_tasksets.size();
	if(nt == 0)
	{
		cout << "file not exists or is empty: " << ss << endl;
		return;
	}
	nt = (*g_vec_tasksets.begin())->vec.size();
	int i;
	statistic  *stats[NUM_DO_CONFIGS];
	for(i=0; i<NUM_DO_CONFIGS; i++) //create results files
	{
		g_config = g_configs[doConfigs[i]];

		ss = to_string(g_config.alg);
		ss += (g_config.merge)?"T":"F";
		ss += (g_config.useoffset)?"T":"F";
		ss += (g_config. occupyshort)?"T":"F";
		stats[i] = new statistic(ss, nt);
	}

	vector<taskset_t *>::iterator itts;
	taskset *ts;
	task *t;
	int endstate = TS_STATE_SCHEDED;
	for(itts=g_vec_tasksets.begin(); itts!=g_vec_tasksets.end(); itts++)
	{
		nt = (*itts)->vec.size();
		ts = new taskset(nt);
		ts->readFromMem(*itts);
		ts->tag = (*itts)->tag;

		//note that tag is ending with /r
		cout << "...working on: " << (*itts)->tag << endl;

		// do this ts on all configs
		for(i=0; i<NUM_DO_CONFIGS; i++)
		{
			int j;
			g_config = g_configs[doConfigs[i]];
			g_stat = stats[i];
			cout << "CONFIG__" << g_stat->tag<<endl;

			if(g_config.useoffset)
			{
				ts->minOffsets = ts->minOffsetsO;
				for(int j=0; j<(int)ts->tq.size(); j++)
				{
					t = ts->tq[j];
					t->offset = t->offsetO;
				}
			}
			else
			{
				ts->minOffsets = ts->minOffsetsZ;
				for(j=0; j<(int)ts->tq.size(); j++)
					ts->tq[j]->offset = 0;
			}

			// do this ts on an algorithm
			switch(g_config.alg)
			{
				case 0:
					endstate = g_optPFRPAsync->sched(ts);
					break;
				case 1:
					endstate = g_directLinkedlist->sched(ts);
					break;
				case 2:
					endstate = g_optPFRPAsyncNoSufficientTest->sched(ts);
					break;
				default:
					endstate = TS_STATE_UNKNOWN;
			}

			if(endstate == TS_STATE_SCHEDED)
				tnts->scheded_num++;
			else
			{
				if(endstate == TS_STATE_UNSCHED)
				{
					tnts->sched_failed_num++;
					if(ts->lmax_fail_at != -1)
						tnts->lmax_failed_task_num++;				
				}
				cout <<tnts->filename<<" now has "<<g_stat->num_tried-g_stat->num_sched<<" of "<<g_stat->num_tried<<" failed tasksets.\n";
			}			
//			g_stat ->write_scheduling(ts->tag, &used_list_head); // write scheduling of this ts
//			g_stat ->write_scheduling_bitmap(ts->tag, &used_list_head); // write scheduling of this ts
			cleanup(); // reset used_list and mmc_nodes
			ts->reset(); // reset ts statistics such as response_time, etc
		}
	}

	for(i=0; i<NUM_DO_CONFIGS; i++)
		delete stats[i];

	delete tnts; // will free g_vec_tasksets
}

void init()
{
	used_list_head.first = NULL;
	used_list_head.last = NULL;
	used_list_head.number_of_nodes = 0;

	g_configs = new config_info[24];
	// 0FFF, 0FFT, 0FTF, 0FTT, 0TFF, 0TFT, 0TTF, 0TTT, 
	// 1FFF, 1FFT, 1FTF, 1FTT, 1TFF, 1TFT, 1TTF, 1TTT, 
	// 2FFF, 2FFT, 2FTF, 2FTT, 2TFF, 2TFT, 2TTF, 2TTT 
	for(int i=0; i<3; i++)
	{		
		for(int j=0; j<2; j++)
		{
			for(int k=0; k<2; k++)
			{
				for(int r=0; r<2; r++)
				{
					int m = i*8+(j*4+k*2+r);
					g_configs[m].alg = i;
					g_configs[m].merge = j?true:false;
					g_configs[m].useoffset = k?true:false;
					g_configs[m].occupyshort = r?true:false; // true: work-conserving, false:non-work-conserving
				}
			}
		}
	}

	g_optPFRPAsync = new optPFRPAsync(&g_config);
	g_optPFRPAsync->setMode(0);
	g_directLinkedlist = new directLinkedlist(&g_config);
	g_optPFRPAsyncNoSufficientTest = new optPFRPAsync(&g_config);
	g_optPFRPAsyncNoSufficientTest->setMode(1);

	g_mymem = new mmc();
	cout << "[" << g_mymem->num_nodes_max << "] nodes allocated.\n";

}

void leave()
{
	delete g_optPFRPAsync;
	delete g_optPFRPAsyncNoSufficientTest;
	delete g_directLinkedlist;
	delete[] g_configs;
	delete g_mymem;
}

void cleanList(head_node *l, int mode)
{
	l->first = NULL;
	l->last = NULL;
	l->number_of_nodes = 0;

	if(mode == 1)
	{
		node *p = l->first, *q;
		while(p)
		{
			q = p->next;
			g_mymem->free_node(p);//delete p;
			p = q;
		}
	}
}

void cleanup()
{
	cleanList(&used_list_head, 0);
	g_mymem->free_all();	
}
