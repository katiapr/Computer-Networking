/*
 * Author:Katia Prigon
 * ID:322088154
 * create on 23/12/2015
 */

#include "threadpool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>//for strings
#include <unistd.h>//using system call functions
#define SUCCESS 0

work_t* dequeue(threadpool*);

threadpool* create_threadpool(int num_threads_in_pool)
{
	if( (num_threads_in_pool <= 0)
			|| (num_threads_in_pool > MAXT_IN_POOL) )
	{
		//the num is illegal
		fprintf(stderr,"the num is illegal\n");
		return NULL;
	}
	threadpool* _pool = (threadpool*)malloc(sizeof(threadpool));
	if(_pool == NULL)
	{
		perror("allocation");
		return NULL;
	}
	//----------------//
	_pool->qsize = 0;
	_pool->qhead = NULL;
	_pool->qtail = NULL;
	_pool->shutdown = 0;
	_pool->dont_accept = 0;
	_pool->num_threads = num_threads_in_pool;
	//---------------//
	if(pthread_mutex_init(&(_pool->qlock),NULL) != SUCCESS)
	{
		fprintf(stderr,"pthread_mutex_init\n");
		free(_pool);
		return NULL;
	}
	if(pthread_cond_init(&(_pool->q_empty),NULL) != SUCCESS)
	{
		fprintf(stderr,"pthread_cond_init\n");
		free(_pool);
		return NULL;
	}
	if (pthread_cond_init(&(_pool->q_not_empty),NULL) !=SUCCESS)
	{
		fprintf(stderr,"pthread_cond_init\n");
		free(_pool);
		return NULL;
	}
	//----------------//
	_pool->threads = (pthread_t*)malloc(sizeof(pthread_t)* _pool->num_threads);
	if(_pool->threads == NULL)
	{
		perror("allocation");
		free(_pool);
		_pool = NULL;
		return NULL;
	}

	int i = 0,status;
	for(i = 0; i< num_threads_in_pool; i++)
	{
		status = pthread_create(_pool->threads+i,NULL,do_work,(void*)_pool);
		if(status)
		{
			fprintf(stderr,"cannot create pthreadpool\n");
			free(_pool->threads);
			free(_pool);
			return NULL;
		}
	}

	return _pool;
}

void dispatch(threadpool* from_me, dispatch_fn dispatch_to_here, void *arg)
{
	if(from_me == NULL || dispatch_to_here == NULL)
	{
		return;
	}
	//critical:
	if(pthread_mutex_lock(&from_me->qlock) != 0)
	{
		fprintf(stderr,"cannot lock\n");
		return;
	}
	if(from_me->dont_accept == 1)
	{
		if(pthread_mutex_unlock(&from_me->qlock) != 0)
		{
			fprintf(stderr,"cannot unlock\n");
			return;
		}
		return;
	}
	//create work_t
	work_t* _work = (work_t*)malloc(sizeof(work_t));
	if(_work == NULL)
	{
		perror("allocation");
		return;
	}
	//init work_t:

	_work->arg = arg;
	_work->routine = dispatch_to_here;
	_work->next = NULL;

	//adding element
	if(from_me->qsize == 0)//empty queue
	{
		from_me->qhead = _work;
		from_me->qtail = _work;
		if(pthread_cond_signal(&(from_me->q_empty)) != SUCCESS)
		{
			fprintf(stderr,"pthread_cond_signal\n");
			free(_work);
			return;
		}
	}
	else if(from_me->qsize > 0)
	{
		from_me->qtail->next = _work;
		from_me->qtail = _work;
	}
	from_me->qsize++;
	if(pthread_mutex_unlock(&from_me->qlock) != 0)
	{
		fprintf(stderr,"cannot unlock\n");
		free(_work);
		return;
	}
	//TODO
	return;
}
void* do_work(void* p)
{
	threadpool* _pool = (threadpool*)p;

	while(1)
	{
		//lock
		if(pthread_mutex_lock(&_pool->qlock) != SUCCESS)

		{
			fprintf(stderr,"pthread_mutex_lock\n");
			return NULL;
		}
		//destruction flag
		if(_pool->shutdown == 1)
		{
			if(pthread_mutex_unlock(&_pool->qlock) != SUCCESS)
			{
				fprintf(stderr,"pthread_mutex_unlock\n");
				return NULL;
			}
			return NULL;
		}
		//empty queue
		if(_pool->qsize == 0)
		{
			//wait till gets work to do
			if(pthread_cond_wait(&_pool->q_empty,&_pool->qlock) != SUCCESS)

			{
				fprintf(stderr,"pthread_cond_wait\n");
				return NULL;
			}
		}
		//destruction flag
		if(_pool->shutdown == 1)
		{
			if(pthread_mutex_unlock(&_pool->qlock) != SUCCESS)
			{
				fprintf(stderr,"pthread_mutex_unlock\n");
				return NULL;
			}
			return NULL;
		}
		//-----------------------------//
		work_t* _work;
		//gets the first element(work_t) from the queue
		_work = dequeue(_pool);
		//empty queue
		if(_pool->qsize == 0)
		{
			if(pthread_cond_signal(&_pool->q_not_empty) != SUCCESS)
			{
				fprintf(stderr,"pthread_cond_signal\n");
				return NULL;
			}
		}
		//unlock
		if(pthread_mutex_unlock(&_pool->qlock) != SUCCESS)
		{
			fprintf(stderr,"pthread_mutex_unlock\n");
			return NULL;
		}
		if(_work == NULL)
		{
			fprintf(stderr,"work_null\n");
			return NULL;
		}
		//TODO
//		printf("before execute\n");
//		if(_work == NULL)
//			printf("work isnt allocated\n");
//		else printf("OK #1\n");
//		if(_work->arg == NULL)
//			printf("work->arg = NULL\n");
//		else printf("OK #2\n");
//		if(_work->routine == NULL)
//			printf("work->routine = NULL\n");
//		else printf("OK #3\n");
//		//		printf("work-> arg = %d",(int*)_work->arg);
		_work->routine(_work->arg);
//		printf("after execute\n");

		free(_work);
	}
}
void destroy_threadpool(threadpool* destroyme)
{

	int i = 0;
	if(destroyme == NULL)
	{
		fprintf(stderr,"destroy is null\n");
		return;
	}
	//lock
	if(pthread_mutex_lock(&destroyme->qlock) != SUCCESS)
	{
		fprintf(stderr,"pthread_mutex_lock\n");
		return;
	}

	destroyme->dont_accept = 1;
	//queue not empty
	if(destroyme->qsize != 0)
	{
		if(pthread_cond_wait(&destroyme->q_not_empty,
				&destroyme->qlock) != SUCCESS)
		{
			fprintf(stderr,"pthread_cond_wait\n");
			return;
		}
	}

	destroyme->shutdown = 1;

	if(pthread_cond_broadcast(&destroyme->q_empty) != SUCCESS)
	{
		fprintf(stderr,"pthread_cond_broadcast\n");
		return;
	}
	//unlock
	if(pthread_mutex_unlock(&destroyme->qlock) != SUCCESS)
	{
		fprintf(stderr,"pthread_mutex_unlock\n");
		return;
	}
	for(i = 0; i< destroyme->num_threads ; i++)
	{
		if(pthread_join(*(destroyme->threads+i),NULL) != SUCCESS)
		{
			fprintf(stderr,"pthread_join\n");
			return;
		}
	}
	free(destroyme->threads);
	//destroys:
	if(pthread_cond_destroy(&destroyme->q_empty) !=SUCCESS)
	{
		fprintf(stderr,"pthread_cond_destroy\n");
		return;
	}
	if(pthread_cond_destroy(&destroyme->q_not_empty) !=SUCCESS)
	{
		fprintf(stderr,"pthread_cond_destroy\n");
		return;
	}

	if(pthread_mutex_destroy(&destroyme->qlock) != SUCCESS)
	{
		fprintf(stderr,"pthread_mutex_destroy\n");
		return;
	}

	free(destroyme);

}
work_t* dequeue(threadpool* _pool)
{

	if(_pool == NULL || _pool->qsize == 0)
	{
		return NULL;
	}
	work_t* temp;
	temp = _pool->qhead;
	//only one
	if(_pool->qsize == 1)
	{

		_pool->qhead = NULL;
		_pool->qtail = NULL;
	}
	//else
	else
	{
		_pool->qhead = _pool->qhead->next;
	}
	_pool->qsize--;

	return temp;
}
