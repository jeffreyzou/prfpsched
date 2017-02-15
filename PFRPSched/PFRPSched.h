#pragma once

#ifdef _THIS_IS_LINUX
#define INT64 int64_t
#else
#include <windows.h>
#endif

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
using namespace std;

//#define MY_DEBUG

//scheduled, 1:un_schedulable, 2: insufficient memory, 3: empty ...
#define TS_STATE_SCHEDED	0
#define TS_STATE_UNSCHED	1
#define TS_STATE_INSUFMEM	2
#define TS_STATE_EMPTY	3
#define TS_STATE_UNKNOWN	4
#define TS_STATE_ERROR		-1

//#define TS_GEN_OFFSET	1

//char action;// 0-done; 1-abort;7-merged
#define TS_ACTION_DONE	0
#define TS_ACTION_ABORT	1
#define TS_ACTION_MERGE	7

class task {
public:
	std::string	name;
	int		c;
	int		incomplete_c;
	int		d;
	int		p; //period
	int		offsetO;
	int		offset;
	int		pri;
	float	util;
	task(std::string _name, int _c, int _d, int _p, int _offset = 0, int _pri = 0) : name(_name), c(_c), d(_d), p(_p), offset(_offset), pri(_pri)
	{
		util = (float)((c*1.0)/p);
		offsetO=_offset;
		resp_time = NULL;
	};
	int *resp_time;
	~task() { if(resp_time) delete resp_time; resp_time=NULL;};
//	bool operator<(const task &rhs) const { return p < rhs.p; };
};

// simple task struct, use less memory than class task
typedef struct _task_t {
	char name[8];
	int		c;
	int		d;
	int		p; //period
	int		offset;
	int		pri;
} task_t, *ptask_t;

typedef struct _taskset_t {
	char tag[16];
	vector<task_t *> vec;
} taskset_t, *ptaskset_t;

typedef struct _node {
	INT64 start_of_interval;
	INT64 end_of_interval;
	struct _node *next;
	char action;// 0-release&finish;1-restart&finish;2-abort;7-merged
	char nTask; 
} node, *pnode;

typedef struct _head_node {
	INT64 number_of_nodes;
	node *first;
	node *last;
	node *prop_s_node; // the first node in a lcm
	string info;
} head_node, *phead_node;

typedef struct _config_info {
	int alg;
	bool merge;
	bool occupyshort;
	bool useoffset;
} config_info, *pconfig_info;

INT64 gcd(INT64 a, INT64 b);
INT64 lcm(INT64 a, INT64 b);

// p == null means insert as l->first to a non-empty list
// p->next might be delete because of merge
// however, p is valid whatever
// return:
//	 0 - nothing change for empty node or no list exist
//	-1 - memory insufficient
//	 1 - ok
int attach_node(head_node *l, node *p, INT64 s, INT64 e, bool merge, char action, char nTask);

// check the linked list, for debug use
// return:
//	 0 - good
//	-1 - something bad
int check_linked_list(head_node *list_head);

