/*
 * Katia Prigon 322088154 SLIST.C
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

// void *slist_pop_first(slist_t * _plist)
// {
// 	if(_plist == NULL)
// 	{
// 		printf("the list is not exist\n");
// 		return NULL;
// 	}
// 	if(slist_head(_plist) == NULL)
// 	{
// 		printf("the list is empty\n");
// 		return NULL;
// 	}
// 	void* data = NULL;
// 	if(slist_size(_plist) == 1)
// 	{
// 		data = slist_data(slist_head(_plist));
		// NEED TO FREE THE HEAD BEFORE ! ! ! that was the problem
// 		slist_head(_plist) = NULL;
// 		slist_tail(_plist) = NULL;
// 		slist_size(_plist) = 0;
// 		//		free(_plist);
// 		return data;

// 	}

// 	slist_node_t *temp;
// 	temp = slist_head(_plist);
// 	slist_head(_plist) = slist_next(slist_head(_plist));
// 	slist_size(_plist)--;
// 	data = slist_data(temp);

// 	free(temp);
// 	return data;
// }

void *slist_pop_first(slist_t * _plist)
{
	if(_plist == NULL)
	{
		printf("the list is not exist\n");
		return NULL;
	}
	if(slist_head(_plist) == NULL)
	{
		printf("the list is empty\n");
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
		printf("cannot create list\n");
		return FAIL;
	}
	if(_pdata == NULL)
	{
		printf("cannot append the node there is no data\n");
		return FAIL;
	}


	slist_node_t *temp = (slist_node_t*)malloc(sizeof(slist_node_t));
	if(temp == NULL)
	{
		free(temp);
		printf("cannot allocate\n");
		return -1;
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
		printf("cannot prepend the node there is no data\n");
		return FAIL;
	}
	slist_node_t * temp = (slist_node_t*)malloc(sizeof(slist_node_t));
	if(temp == NULL)
	{
		free(temp);
		printf("cannot allocate\n");
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
////////////
//TODO
////////////
void slist_print(slist_t* _plist)
{
	if(_plist == NULL)
	{
		printf("the list is empty\n");
		return;
	}
	slist_node_t* p ;
	p=_plist->head;
	for( ; p!=NULL; p = p->next)
	{
		if(p->next == NULL) printf("%p -> NULL\n",p->data);
		else printf("%p -> ",p->data);
	}
}

//int main()
//{
//
//
//	printf("\t**welcome to the main**\n");
//	//-//first list:
//	slist_t *_plist1 = (slist_t*)malloc(sizeof(slist_t));
//	if(_plist1 == NULL) {printf("list is NULL\n");
//	return -1;}
//	slist_init(_plist1);
//	printf("after init\n");
//	void* a ;
//	a = (void*)1;
//	slist_append(_plist1,a);
//	a = (void*)2;
//	slist_append(_plist1,a);
//	a = (void*)3;
//	slist_append(_plist1,a);
//	printf("first list is\n");
//	slist_print(_plist1);
//	printf("after print\n");
//	a = (void*)1;
//	slist_append(_plist1,a);
//	printf("first list is");
//	slist_print(_plist1);
//	//second list:
//	printf("*****************SECOND******************************\n");
//	slist_t *_plist2 = (slist_t*)malloc(sizeof(slist_t));
//	if(_plist2 == NULL){printf("list is NULL\n");
//	return -1;}
//	slist_init(_plist2);
//	a = (void*)4;
//	slist_append(_plist2,a);
//	a = (void*)5;
//	slist_append(_plist2,a);
//	a = (void*)6;
//	slist_append(_plist2,a);
//
//	printf("first list is");
//	slist_print(_plist1);
//	printf("\nsecond list is:");
//	slist_print(_plist2);
//	printf("\n");
//
//	printf("pop:\n");
//	int i ;
//	printf("sizeof plist1: %d\n",slist_size(_plist1));
//	int size;
//	size=slist_size(_plist1);
//	for(i = 0; i <size;i++)
//	{
//		printf("value is: %p\n",slist_pop_first(_plist1));
//	}
//	printf("size of _plist1: %d\n",slist_size(_plist1));
//	printf("_plist1 is empty\n");
//	printf("print list(empty) _plist\n");
//	slist_print(_plist1);
//	printf("append second to first list is");
//	slist_append_list(_plist1,_plist2);
//	slist_print(_plist1);
//	//------//
//	printf("*****************SECOND******************************\n");
//	slist_t* _plist3 = (slist_t*)malloc(sizeof(slist_t));
//	slist_init(_plist3);
//	slist_append_list(_plist1,_plist3);
//	printf("_plist1\n");
//	slist_print(_plist1);
//
//	slist_append_list(_plist3,_plist1);
//	printf("_plist3:\n");
//	slist_print(_plist3);
//	//	printf("size of this list is : %d", _plist1->size);
//	//	slist_pop_first(_plist1);
//	//	printf("size of this list is : %d", _plist1->size);
//	printf("\nBYE BYE here the free\n");
//	//	slist_print(_plist1);
//
//	slist_destroy(_plist1,SLIST_LEAVE_DATA);
//	slist_destroy(_plist2,SLIST_LEAVE_DATA);
//	slist_destroy(_plist3,SLIST_LEAVE_DATA);
//	printf("---------------------------------------------------------------------\n");
//
//	return 0;
//}