/**
 * @Author Sahil Patel (spatel32@umbc.edu)
 * This file contains a program that gets two arguements from the command line
 * and parses the information to create optimal trick-or-treating routes
 *
 * https://stackoverflow.com/questions/13145777/c-char-to-int-conversion
 * https://www.tutorialspoint.com/cprogramming/index.htm
 * Manpages!!
 * https://stackoverflow.com/questions/868496/how-to-convert-char-to-integer-in-c
 * https://stackoverflow.com/questions/8773346/how-to-initialize-a-dynamic-int-array-elements-to-0-in-c
 * https://stackoverflow.com/questions/15340415/unsigned-integers-in-c
 * https://stackoverflow.com/questions/12990723/dynamic-array-of-structs-in-c
 * https://stackoverflow.com/questions/14888027/mutex-lock-threads
 * https://stackoverflow.com/questions/1352749/multiple-arguments-to-function-called-by-pthread-create
 * 
 *
 * Q: While the main thread is displaying the current state of all children groups, there is [at least one] potential race condition. Describe a scenario that could trigger that race condition.
 * A: While the main thread is displaing the current state of all the children groups, the children threads could be in the process of getting candy from the houses OR the neighborhood thread could be refilling the candy in specific houses. 
 * 
 * Q: How could your program prevent that race condition? No code necessary, just describe what you would do.
 * A: You would lock out the thread everytime you would print out data to prevent for cross contamination of the data.
 */

#define _BSD_SOURCE || _XOPEN_SOURCE >= 500
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct house
{
    unsigned int houseNumber;
    unsigned int posX;
    unsigned int posY;
    unsigned int candyCount;
    pthread_mutex_t lock;
};

struct group
{
    int groupNumber;
    unsigned int posX;
    unsigned int posY;
    int startHouse;
    int currHouse;
    int numOfChildren;
    unsigned int candyCounter;
};

int T;
int timer;
int G;
int LINE_LENGTH = 80;
int NUM_HOUSES = 10;

struct house houses[10];

struct group *groups;
pthread_t *tids;

/**
 * manhattanDistance() - Calculate the time for child to move to next house.
 *
 * @groupX: X position of group
 * @groupY: Y position of group
 * @houseX: X position of house
 * @houseY: Y position of house
 * Return: Manhattan distance multiplied by 250 milliseconds
 */
int manhattanDistance(unsigned int groupX, unsigned int groupY, unsigned int houseX, unsigned int houseY)
{
    unsigned int distance = abs(groupX - houseX) + abs(groupY - houseY);
    return (distance * 250);
}

/**
 * childrenGroup() - start routine function for the children threads
 *
 * @args: arguements that are passed into the start routine
 * Return: NULL
 */
static void *childrenGroup(void *args)
{
    struct group *child = args;
    
    while(timer != (T * 1000))
    {
	// algorithm to find nearest house with enough candy for group
	int minDistance = (T * 1000);
	int maxCandy = 0;
	int houseNum = 0;
	for(int i = 0; i < NUM_HOUSES; i++)
	{
	    if(i != child->startHouse && i != child->currHouse)
	    {
		int distance = manhattanDistance(child->posX, child->posY, houses[i].posX, houses[i].posY);
		int candyReserve = houses[i].candyCount;
		if( distance < minDistance  && candyReserve > maxCandy )
		{
		    minDistance = distance;
		    maxCandy = candyReserve;
		    houseNum = i;
		}
	    }
	}
	
	usleep(minDistance * 1000);
	pthread_mutex_lock(&houses[houseNum].lock);
	if(houses[houseNum].candyCount < child->numOfChildren)
	{
	    child->candyCounter += houses[houseNum].candyCount;
	    houses[houseNum].candyCount = 0;
	}
	else
	{
	    child->candyCounter += child->numOfChildren;
	    houses[houseNum].candyCount -= child->numOfChildren;
	}

	child->posX = houses[houseNum].posX;
	child->posY = houses[houseNum].posY;
	child->currHouse = houseNum;
	
	pthread_mutex_unlock(&houses[houseNum].lock);
	
    }
    
    return NULL;
}

/**
 * neighborhood() - start routine function for the neighborhood thread
 *
 * @args: arguements that are passed into the start routine
 * Return: NULL
 */
static void *neighborhood(void *args)
{
    FILE *file = args;
    char str[LINE_LENGTH];

    while( fgets(str, LINE_LENGTH, file) != NULL && timer != (T * 1000))
    {
	usleep(250 * 1000);
	int house = str[0] - '0';
	int refill = str[2] - '0';
	pthread_mutex_lock(&houses[house].lock);
	houses[house].candyCount += refill;
	pthread_mutex_unlock(&houses[house].lock);
    }

    return NULL;
}

int main( int argc, char *argv[] )
{

   if( argc == 3 )
   {
       T = atoi(argv[2]);
       timer = 0;
       FILE *fp = fopen(argv[1], "r");
       char str[LINE_LENGTH];

       if( fgets(str, LINE_LENGTH, fp) != NULL)
       {
	   G = atoi(str);
       }

       for(int i = 0; i < 10; i++)
       {
	   char str[LINE_LENGTH];
	   if( fgets(str, LINE_LENGTH, fp) != NULL)
	   {
	       houses[i].houseNumber = i;
	       houses[i].posX = (int) str[0] - '0';
	       houses[i].posY = (int) str[2] - '0';
	       houses[i].candyCount = (int) str[4] - '0';
	       pthread_mutex_init(&houses[i].lock, NULL);
	   }
	   else
	   {
	       i = 10;
	   }
       }
       
       groups = (struct group*) malloc(G*sizeof(struct group));
       tids = (pthread_t*) malloc(G*sizeof(pthread_t));
       for(int i = 0; i < G; i++)
       {
	   char str[LINE_LENGTH];
	   if( fgets(str, LINE_LENGTH, fp) != NULL)
	   {
	       char numOfChildrenStr[LINE_LENGTH];
	       int numOfChildrenIndex = 0;
	       for(int j = 1; j < LINE_LENGTH; j++)
	       {
		   if(str[j] == '\0')
		   {
		       j = LINE_LENGTH;
		   }
		   else if(str[j] != ' ' && str[j] != '\n')
		   {
		       numOfChildrenStr[numOfChildrenIndex] = str[j];
		       numOfChildrenIndex++;
		   }
	       }
	       
	       int houseNum = (int) str[0] - '0';
	       
	       groups[i].groupNumber = i;
	       groups[i].posX = houses[houseNum].posX;
	       groups[i].posY = houses[houseNum].posY;
	       groups[i].startHouse = houseNum;
	       groups[i].currHouse = houseNum;
	       groups[i].numOfChildren = atoi(numOfChildrenStr);
	       groups[i].candyCounter = 0;
	       
	       if( pthread_create(tids + i, NULL, childrenGroup, groups + i) != 0)
	       {
		   printf("Thread creation failed!");
		   return -1;
	       }
	   }
	   else
	   {
	       i = G;
	   }
	   
       }

       pthread_t neighborhood_tids;
       if( pthread_create(&neighborhood_tids, NULL, neighborhood, fp) != 0)
       {
	   printf("Thread creation failed!");
	   return -1;
       }

       //pthread_join(neighborhood_tids, NULL);

       for(int i = 0; i <= T; i++)
       {
	   
	   int totalCandy = 0;
	   printf("After %d seconds: \n", i);
	   printf("\tGroup statuses: \n");
	   for(int j = 0; j < G; j++)
	   {
	       printf("\t\t%d: Size: %d, going to %d, collected %d \n", j, groups[j].numOfChildren, groups[j].currHouse, groups[j].candyCounter);
	       totalCandy += groups[j].candyCounter;
	   }
	   printf("\tHouse statuses: \n");
	   for(int j = 0; j < NUM_HOUSES; j++)
	   {
	       printf("\t\t%d @ (%d,%d): %d available \n", j, houses[j].posX, houses[j].posY, houses[j].candyCount);
	   }

	   printf("\tTotal candy: %d \n", totalCandy);
	   
	   
	   usleep(1000 * 1000);
	   timer += 1000;
       }

       fclose(fp);
   }
   else if( argc > 3 )
   {
      printf("Too many arguments supplied.\n");
   }
   else
   {
      printf("Too few argument provided.\n");
   }
}
