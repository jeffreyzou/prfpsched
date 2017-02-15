#include "stdafx.h"
#include "mmc.h"

#ifdef _THIS_IS_LINUX
#include <cmath>
#endif

mmc::mmc(void)
{
	INT64 max = 4 * (INT64)pow(10, 7);
	num_failed_alloc = 0;
	num_failed_free = 0;

	preusable = new node*[max];
	num_reusable_max = max;
	num_reusable = 0;
	cout <<  "memory size 1:" << max*(sizeof(node*)) << endl;

	cout <<  "memory size 2:" << max*(sizeof(node)) << endl;

	num_nodes = 0;
	num_nodes_max = max;
	pnodes = new node[max];
	if(!pnodes)
	{
		num_nodes_max = 0;
	}
/*
	num_nodes_max = 0;
	ppnodes = new node*[max];
	while(num_nodes_max < max)
	{
		ppnodes[num_nodes_max] = new node;
		if(!ppnodes[num_nodes_max])
			break;
		num_nodes_max++;
	}
*/
	num_remains = num_nodes_max;
}


mmc::~mmc(void)
{
	num_nodes = 0;
/*
	while(num_nodes_max >= 0)
	{
		num_nodes_max--;
		delete ppnodes[num_nodes_max];
	}
	delete ppnodes;
*/
	delete[] pnodes;
	pnodes = NULL;

	num_reusable = 0;
	num_reusable_max = 0;
	delete preusable;
}

node* mmc::alloc_node()
{
	if(num_reusable)
	{
		num_remains--;
		num_reusable --;
		return preusable[num_reusable];
	}

	if(num_nodes == num_nodes_max)
	{
		num_failed_alloc++;
		return NULL;
	}

	num_remains--;
	num_nodes += 1;
//	return ppnodes[num_nodes-1];
	return &pnodes[num_nodes-1];
}

int mmc::free_node(node *p)
{
	if(num_reusable < num_reusable_max)
	{
		preusable[num_reusable] = p;
		num_reusable++;
		num_remains++;
		return 1;
	}
	num_failed_free++;
	std::cout << "!!!! resuable pool is full!!!\n";
	return 0;
}

int mmc::free_all()
{
	num_reusable = 0;
	num_nodes = 0;
	num_remains = num_nodes_max;
	num_failed_alloc = 0;
	num_failed_free = 0;
	return 1;
}
