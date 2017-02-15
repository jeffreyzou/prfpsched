#pragma once

#include "PFRPSched.h"

class mmc
{
public:
	node **ppnodes;
	node *pnodes;
	INT64 num_nodes;
	INT64 num_nodes_max;
	node **preusable;
	INT64 num_reusable;
	INT64 num_reusable_max;

	INT64 num_remains;

	INT64 num_failed_alloc;
	INT64 num_failed_free;

	node* alloc_node();
	int free_node(node *p);
	int free_all();

	mmc(void);
	~mmc(void);
};

