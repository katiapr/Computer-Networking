// ---------------------------
#include <stdio.h>
#include <stdlib.h>
#include "pattern_matching.h"
// ---------------------------



  /*********************************************************************/
 /****************************** TESTER: ******************************/
/*********************************************************************/



int main()
{	
	printf("\n  // ************************************************** //\n");
	printf(" // ***************** TESTER FOR EX1 ***************** //");
	printf("\n// ************************************************** //\n");


	// Allocation of the FSM structure:
	pm_t* fsm = (pm_t*)malloc(sizeof(pm_t));
	if (!fsm)
	{
		printf("FSM Allocation Failed!\n");
		exit(-1);
	}


	// Checking the pm_init - Initialize the FSM structure:
	int pm_init_value = pm_init(fsm);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// * * * * * TEST THE SECOND EXAMPLE OF THE FSM: * * * * * //

	printf("\nAccording to the SECOND example:\n\n");

	// Adding the SECOND example patterns:
	printf("\npm_addstring: \n\n");

	pm_addstring(fsm, "a" , 1);
	pm_addstring(fsm, "abc" , 3);
	pm_addstring(fsm, "bca" , 3);
	pm_addstring(fsm, "cab" , 3);
	pm_addstring(fsm, "acb" , 3);

	// Make FSM function - Updating the failure pointers and the output list of each state:
	printf("\npm_makeFSM: \n\n");
	pm_makeFSM(fsm);

	// * * * CHECKING FOR THE OUTPUT LIST * * * // Printing the output of each state - Starting from s(1):

	printf("\npm_fsm_search: \n\n");
	slist_t* matches = pm_fsm_search(fsm->zerostate, "xyzabcabde", 10);

	// Destroy Function - Freeing all the allocated memory used to build the fsm.
	pm_destroy(fsm);
	printf("\n");


	// * * * * * END OF THE SECOND EXAMPLE * * * * * //
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	// * FREE ALL ALLOCATION OF THE MAIN * //
	slist_destroy(matches, SLIST_FREE_DATA);
	free(fsm);

	return 0;
}


 // * END OF THE MAIN * //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

