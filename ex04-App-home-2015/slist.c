/*
 * Author: Katia Prigon
 * login: katiapr
 * ID:322088154
 * SLIST.c
 */
#include "slist.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

//-------------
static const int FAIL = -1;
static const int SUCCESS = 0;
//-------------
//void slist_print(slist_t* _plist);//TODO
void slist_node_destroy(slist_node_t*, slist_destroy_t);//mine func to implement frees
//-------------------------------------------//
void slist_init(slist_t * _plist)
{

	slist_head(_plist) = NULL;
	slist_tail(_plist) = NULL;
	slist_size(_plist) = 0;
}
void slist_destroy(slist_t * _plist,slist_destroy_t _flag)
{
	if(_plist)
	{
		slist_node_destroy(_plist->head,_flag);
		free(_plist);
	}

}
//mine func to remove each node:
void slist_node_destroy(slist_node_t *_pnode,slist_destroy_t flag)
{
	if(_pnode == NULL)
		return;
	slist_node_destroy(slist_next(_pnode),flag);
	if(flag == SLIST_FREE_DATA)
	{
		free(slist_data(_pnode));
	}
	free(_pnode);
	_pnode = NULL;
}
void *slist_pop_first(slist_t * _plist)
{
	if(_plist == NULL)
	{
		return NULL;
	}
	if(slist_head(_plist) == NULL)
	{
		return NULL;
	}

	// At least 1 node is exist:
	void* data = slist_data(slist_head(_plist));
	slist_node_t *temp = slist_head(_plist);

	if(slist_size(_plist) == 1)
	{
		slist_head(_plist) = NULL;
		slist_tail(_plist) = NULL;
	}
	else
		slist_head(_plist) = slist_next(slist_head(_plist));

	slist_size(_plist)--;
	free(temp);

	return data;
}

int slist_append(slist_t * _plist,void* _pdata)
{
	//allocation
	if(_plist == NULL)
	{
		return FAIL;
	}
	if(_pdata == NULL)
	{
		return FAIL;
	}


	slist_node_t *temp = (slist_node_t*)malloc(sizeof(slist_node_t));
	if(temp == NULL)
	{
		return FAIL;
	}

	slist_data(temp) = _pdata;
	slist_next(temp) = NULL;
	if(slist_size(_plist) == 0)//empty list
	{
		slist_head(_plist) = temp;
		slist_tail(_plist) = temp;
		slist_size(_plist)++;
		return SUCCESS;
	}



	slist_next(slist_tail(_plist)) = temp;
	slist_tail(_plist) = temp;
	slist_size(_plist)++;
	return SUCCESS;
}
int slist_prepend(slist_t * _plist,void * _pdata)
{
	if(_pdata == NULL)
	{
		return FAIL;
	}
	slist_node_t * temp = (slist_node_t*)malloc(sizeof(slist_node_t));
	if(temp == NULL)
	{
		return FAIL;
	}
	slist_data(temp) = _pdata;
	slist_next(temp) = NULL;
	//list is empty:
	if(slist_head(_plist) == NULL)
	{
		slist_head(_plist) = temp;
		slist_tail(_plist) = temp;
	}
	else
	{
		slist_next(temp) = slist_head(_plist);
		slist_head(_plist) = temp;
	}
	slist_size(_plist)++;

	return SUCCESS;
}
/** \brief Append elements from the second list to the first list, use the slist_append function.
	you can assume that the data of the lists were not allocated and thus should not be deallocated in destroy
	(the destroy for these lists will use the SLIST_LEAVE_DATA flag)
	\param to a pointer to the destination list
	\param from a pointer to the source list
	\return 0 on success, or -1 on failure
 */
int slist_append_list(slist_t* des, slist_t* sour)
{
	if(sour->size == 0)//second list is empty
		return 0;
	slist_node_t * temp;
	for(temp = slist_head(sour); temp!=NULL; temp = slist_next(temp))
	{
		if(slist_append(des,slist_data(temp)) == FAIL)
			return FAIL;
	}
	return SUCCESS;

}

