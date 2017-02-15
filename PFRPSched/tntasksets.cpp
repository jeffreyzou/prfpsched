#include "stdafx.h"
#include "tntasksets.h"
#ifdef _THIS_IS_LINUX
#include <string.h>
#endif
#include <algorithm>

bool sortByP(const task_t* lhs, const task_t* rhs) 
{ 
    return lhs->p < rhs->p;
}
bool sortByC(const task_t* lhs, const task_t* rhs) 
{ 
    return lhs->c < rhs->c;
}
bool sortByU(const task_t* lhs, const task_t* rhs) 
{ 
	return lhs->c/lhs->d < rhs->c/lhs->d;
}

tntasksets::tntasksets(void)
{
	lmax_failed_task_num = 0;
	wcrt_failed_task_num = 0;
	scheded_num = 0;
	sched_failed_num = 0;
	mem_failed_num = 0;
}


tntasksets::~tntasksets(void)
{
	vector<taskset_t *> ::iterator itts = _vec_tasksets->begin(); 
	vector<task_t *>::iterator itt;
	for(; itts != _vec_tasksets->end(); itts++)
	{
		itt = (*itts)->vec.begin();
		for(; itt != (*itts)->vec.end(); itt++)
			delete (*itt);
	}
}

task_t * tntasksets::parseLine(char *line)
{
	if(strlen(line) == 0)
		return NULL;

	char * pch;
	int i;
	char *str[5];
	char *next_token = NULL;
	int num_para;

	num_para = 5;
#ifdef _THIS_IS_LINUX
	pch = strtok_r(line, ",", &next_token);
#else	
	pch = strtok_s(line, ",", &next_token);
#endif
	i = 0;
	while (pch != NULL)
	{
		str[i] = pch;
#ifdef _THIS_IS_LINUX
		pch = strtok_r(NULL, ",", &next_token);
#else	
		pch = strtok_s(NULL, ",", &next_token);
#endif
		i++;
		if(i==num_para)
			break;
	}
	//ID,Priority,Offset,Period, Execution Time
	//T1,3,1,147,58
	task_t *t = new task_t;
	if(!t)
		return NULL;

	if(strlen(str[0]) > sizeof(t->name))
	{
#ifdef _THIS_IS_LINUX
		strncpy(t->name, str[0], 7);
#else	
		strcpy_s(t->name, 7, str[0]);
#endif		
		t->name[8] = '\0';
	}
	else
	{
#ifdef _THIS_IS_LINUX
		strcpy(t->name, str[0]);
#else	
		strcpy_s(t->name, str[0]);
#endif
		t->name[strlen(str[0])]='\0';
	}
	t->c = atoi(str[4]);
	t->p = atoi(str[3]);
	t->d = t->p;
	t->offset = atoi(str[2]);
	t->pri = atoi(str[1]);

	return t;
}

int tntasksets::readTasksetsFromFile(string filepath, vector<taskset_t *> *vec_tasksets)
{
	filename = filepath;
	_vec_tasksets = vec_tasksets;
	ifstream fin(filepath);
	char chline[128];
	if(!fin.good())
	{
		cout << "file not found:" << filepath << endl;
		return -1;
	}
	
	int i;
	// first 3 lines
	fin.getline(chline, sizeof(chline));
	// num_task_per_taskset=??
	i = sizeof("num_task_per_taskset=") - 1;
	char *tline = &chline[i];
	int num_task_per_taskset = atoi(tline);
	// num_taskset=??
	fin.getline(chline, sizeof(chline));
	i = sizeof("num_taskset=") - 1;
	tline = &chline[i];
	int num_tasksets = atoi(tline);
	////ID,Priority,Offset,Period, Execution Time
	fin.getline(chline, sizeof(chline));

	taskset_t *taskset;
	task_t *t;

	while((num_tasksets>0) && !fin.eof())
	{
		// tag
		taskset = new taskset_t;
		fin.getline(taskset->tag, 16);
		i = 0;
		while(i < num_task_per_taskset)
		{
			// task
			fin.getline(chline, 128);
			t = parseLine(chline);
			if(!t)
				return 0;
			taskset->vec.push_back(t);
			i++;
		}
		// no task
		if(taskset->vec.size() == 0)
			return 0;
		// sort task by period
//		sort(taskset->vec.begin(), taskset->vec.end(), sortByP);

		_vec_tasksets->push_back(taskset);

		num_tasksets--;
	}

	fin.close();	

	return 1;
}
