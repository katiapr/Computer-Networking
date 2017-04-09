#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pattern_matching.h"

//------Mine defines---------//
static const int FAIL = -1;
static const int SUCCESS = 0;
int pm_init_state(pm_state_t* state);
//-------------------------//
/* Initializes the fsm parameters (the fsm itself sould be allocated).  Returns 0 on success, -1 on failure.
 *  this function should init zero state
 */
int pm_init(pm_t * fsm)
{
	fsm->newstate = 0;
	//creation zero state
	fsm->zerostate = (pm_state_t*)malloc(sizeof(pm_state_t));
	if(fsm->zerostate == NULL)
	{
		free(fsm->zerostate);
		printf("pm init note: cannot allocate\n");
		return FAIL;
	}
	pm_init_state(fsm->zerostate);
	fsm->zerostate->fail = NULL;

	return SUCCESS;
}

/* Adds a new string to the fsm, given that the string is of length n.
   Returns 0 on success, -1 on failure.*/
int pm_init_state(pm_state_t* state)
{
	state->_transitions = (slist_t*)malloc(sizeof(slist_t));
	if(state->_transitions == NULL)
	{
		free(state->_transitions);
		printf("pm init note: cannot allocate\n");
		return FAIL;
	}
	slist_init(state->_transitions);
	//----//
	state->output = (slist_t*)malloc(sizeof(slist_t));
	if(state->output == NULL)
	{
		free(state->output);
		printf("pm init note: cannot allocate\n");
		return FAIL;
	}
	slist_init(state->output);
	//---//
	state->depth = 0;
	state->id = 0;
	state->fail = NULL;
	return SUCCESS;
}
int pm_addstring(pm_t * fsm,unsigned char * symbol, size_t length)
{

	if(fsm == NULL)
	{
		printf("pm_addstring note: there is no diagram\n");
		return -1;
	}
	//allocation
	pm_state_t* curr = fsm->zerostate;
	pm_state_t* ptr;

	int i = 0;

	/*updating the current state if there were arrows with prefix of
	 the string */
	while((ptr = pm_goto_get(curr,symbol[i]))!= NULL)
	{
		curr = ptr;
		i++;
	}
	//there is no arrows with the same label as a char
	for(;i < length ; i++)
	{
		//////////////////////////////////////////////////////////////////////
		pm_state_t* to_state =(pm_state_t*)malloc(sizeof(pm_state_t));
		if(to_state == NULL)
		{
			free(to_state);
			printf("pm_addstring note: cannot allocate\n");
			return FAIL;
		}
		//inits:
		if(pm_init_state(to_state) == FAIL)
		{
			free(to_state);
			return FAIL;
		}
		/////////////////////////////////////////////////////////////////////
		if(pm_goto_set(curr,symbol[i],to_state) == FAIL)
		{
			free(to_state);
			return FAIL;
		}
		//TODO
		fsm->newstate++;
		to_state->id = fsm->newstate;
		to_state->depth = curr->depth+1;

		//output://TODO
		printf("Allocate state %d\n",to_state->id);
		printf("%d -> %c -> %d\n",curr->id,symbol[i],to_state->id);
		curr = to_state;
	}
	//update output:
	//	char* str = ( char*)malloc(sizeof( char)*length + 1);
	//	strcpy(str,(char*)symbol);
	if(slist_append(curr->output,symbol) ==  FAIL)
		return FAIL;
	//TODO
	// if(curr->output != NULL)
	// 	printf("get state: %d\n",curr->id);
	return SUCCESS;
}

/* Finalizes construction by setting up the failrue transitions, as
   well as the goto transitions of the zerostate.
   Returns 0 on success, -1 on failure.*/
int pm_makeFSM(pm_t *fsm)
{
	if(fsm == NULL)
	{
		printf("pm_makeFSM note: the pm is empty\n");
		return FAIL;
	}
	//inits for first level:
	slist_t* queue = (slist_t*)malloc(sizeof(slist_t));
	if(queue == NULL)
	{
		free(queue);
		return FAIL;
	}
	slist_init(queue);
	pm_state_t* state;

	//states with depth 1 -> fail = zerostate
	int  i = 0;
	for(i = 0; i < PM_CHARACTERS;i++)
	{

		if((state = pm_goto_get(fsm->zerostate,i)) != NULL)
		{
			if(slist_append(queue,state) == FAIL )
			{
				slist_destroy(queue,SLIST_LEAVE_DATA);
				return FAIL;
			}
			state->fail = fsm->zerostate;
		}
		else
		{
			if(pm_goto_set(fsm->zerostate,i,fsm->zerostate) == FAIL)
				return FAIL;
		}
	}

	///////////////////
	//other levels:////
	///////////////////
	pm_state_t* curr;
	pm_state_t* s;
	while(slist_size(queue) > 0)
	{
		curr = (pm_state_t*)slist_pop_first(queue);
		//TODO
		for(i = 0; i < PM_CHARACTERS; i++)
		{

			if((s = pm_goto_get(curr,i)) != NULL)
			{
				if(slist_append(queue,s) == FAIL)
				{
					slist_destroy(queue,SLIST_LEAVE_DATA);
					return FAIL;
				}
				state = curr->fail;//state = (0)
				//looking for the suffix
				while( state != NULL
						&& pm_goto_get(state,i) == NULL)
				{
					state = state->fail;
				}
				if( state == NULL)
					s->fail = fsm->zerostate;
				else s->fail = pm_goto_get(state,i);
				if(slist_append_list(s->output,s->fail->output) == FAIL)
				{
					slist_destroy(queue,SLIST_LEAVE_DATA);
					return FAIL;
				}
				if(s->fail->id != 0)
					printf("Setting f(%d) = %d\n", s->id, s->fail->id);
			}

		}
	}
	slist_destroy(queue,SLIST_FREE_DATA);
	return SUCCESS;
}


/* Set a transition arrow from this from_state, via a symbol, to a
   to_state. will be used in the pm_addstring and pm_makeFSM functions.
   Returns 0 on success, -1 on failure.*/
//TODO
int pm_goto_set(pm_state_t *from_state,
		unsigned char symbol,
		pm_state_t *to_state)
{
	if(from_state == NULL || to_state == NULL)
	{
		printf("**pm_goto_set note: one of states is not exist\n");
		return -1;
	}

	//------creation of 'data'------//
	pm_labeled_edge_t* temp = (pm_labeled_edge_t*)malloc(sizeof(pm_labeled_edge_t));
	if(temp == NULL)
	{

		free(temp);
		printf("pm_goto_set note: cannot alloacate\n");
		return -1;
	}
	temp->label = symbol;
	temp->state = to_state;
	//------------------------------//

	slist_append(from_state->_transitions,temp);
	//updating depth
	//TODO


	return 0;
}

/* Returns the transition state.  If no such state exists, returns NULL.
   will be used in pm_addstring, pm_makeFSM, pm_fsm_search, pm_destroy functions. */
pm_state_t* pm_goto_get(pm_state_t *state,
		unsigned char symbol)
{
	if(state == NULL)
	{
		printf("pm_goto_get note: the state is not exist\n");
		return NULL;
	}
	slist_node_t* temp;
	for(temp = state->_transitions->head; temp!= NULL; temp = temp->next)
	{
		if(((pm_labeled_edge_t*)temp->data)->label == symbol)
		{
			//pointer to that state:
			return ((pm_labeled_edge_t*)temp->data)->state;
		}
	}
	return NULL;

}



/* Search for matches in a string of size n in the FSM.
   if there are no matches return empty list */
slist_t* pm_fsm_search(pm_state_t * pstate,
		unsigned char * text,size_t n)
{
	if(pstate == NULL)
		return NULL;
	if(text == NULL)
		return NULL;
	slist_t* matched_list = (slist_t*)malloc(sizeof(slist_t));
	if(matched_list == NULL)
	{
		free(matched_list);
		return NULL;
	}
	slist_init(matched_list);
	pm_state_t* state = pstate;
	///////////////////////////////////
	int i;
	for(i = 0; i< n; i++)
	{

		while( state != NULL  &&  pm_goto_get(state,text[i]) == NULL  )
		{
			state = state->fail;
		}
		if(state != NULL)
			state = pm_goto_get(state,text[i]);

		else
			continue;
		if(state->output->size > 0)
		{
			slist_node_t* pattern  = state->output->head;

			// printf("size of pattern : %d\n",state->output->size);
			while(pattern != NULL)
			{
				int len = strlen((char*)pattern->data);
				pm_match_t* matched = (pm_match_t*)malloc(sizeof(pm_match_t));
				if(matched == NULL)
				{
					slist_destroy(matched_list,SLIST_FREE_DATA);
					free(matched);
					return NULL;
				}
				matched->end_pos = i;
				matched->start_pos = i - len + 1;
				matched->fstate = state;
				matched->pattern = (char*)pattern->data;

				if(slist_append(matched_list,matched) == FAIL)
					return NULL;
				//output:
				printf("Pattern: %s, start at: %d,ends at: %d,accepting state = %d\n",
						matched->pattern,matched->start_pos,matched->end_pos,state->id);
				pattern = pattern->next;
			}
		}

	}
	return matched_list;
}

/* Destroys the fsm, deallocating memory. */
void pm_destroy(pm_t * fsm)
{

	if(fsm == NULL)
		return;
	///////----------///
	slist_t* queue = (slist_t*)malloc(sizeof(slist_t));
	if(queue == NULL)
	{
		free(queue);
		return;
	}
	slist_init(queue);
	///states with depth 1
	pm_state_t* curr;
	pm_state_t* s;
	pm_state_t* state;
	int i;
	//insertion states with depth 1
	for(i = 0; i< PM_CHARACTERS; i++)
	{
		//TODO
		//depth = 1

		if((state = pm_goto_get(fsm->zerostate,i)) != NULL && state != fsm->zerostate)
		{
			if(slist_append(queue,state) == FAIL)
			{
				slist_destroy(queue,SLIST_LEAVE_DATA);
				return;
			}
		}

	}

	while(slist_size(queue) > 0)
	{
		curr = (pm_state_t*)slist_pop_first(queue);
		for(i = 0; i< PM_CHARACTERS; i++)
		{

			if( (s = pm_goto_get(curr,i)) != NULL)
			{
				if( slist_append(queue,s) == FAIL)
				{
					free(queue);
					return;
				}
			}
		}
		//TODO
		// printf("now curr is : %d\n",curr->id);
		slist_destroy(curr->_transitions,SLIST_FREE_DATA);
		slist_destroy(curr->output,SLIST_LEAVE_DATA);
		free(curr);

	}
	slist_destroy(fsm->zerostate->_transitions,SLIST_FREE_DATA);
	slist_destroy(fsm->zerostate->output,SLIST_LEAVE_DATA);
	slist_destroy(queue,SLIST_FREE_DATA);
	free(fsm->zerostate);
	// free(fsm);
}