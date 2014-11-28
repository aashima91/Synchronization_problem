#include<iostream>
#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#define DEFAULTTHRESHOLD 25
#define DOORS 4
#define RANDMAX 500

using namespace std;

pthread_t hero;
pthread_t frnd[DOORS];
pthread_mutex_t liveZombieLock;
pthread_mutex_t thresholdMutex;
pthread_mutex_t letInMutex[DOORS];
pthread_cond_t thresholdCrossed;
pthread_cond_t letInMore[DOORS];
long liveZombies, threshold;
bool waitFlag = false;
bool waiting[DOORS];
bool activateDoors[DOORS];

//Function to kill zombie

void killZombie()
{
	pthread_mutex_lock( &liveZombieLock );				//Acquiring lock to modify 'liveZombies' variable
	printf( "Total no of live zombies now = %d \n", liveZombies );	
	liveZombies--;							//Killing one zombie, aka, reducing 'liveZombies' count
	printf( "Killed one zombie\n" );
	pthread_mutex_unlock( &liveZombieLock );			//Releasing lock
}

//Function to let in a zombie through a door

void letInZombie()
{
	pthread_mutex_lock( &liveZombieLock );				//Acquiring lock to modify 'liveZombies' variable
	liveZombies++;							//Letting in a zombie, aka, incrementing 'liveZombies' count
	pthread_mutex_unlock( &liveZombieLock );			//Releasing lock
}

//Hero Thread

void *heroThread( void *n )
{
	int counter=0;							//Keeping track of 10ms sleeps
	int killCount = 0;						//Number of zombies killed
	int randNum;							//Variable to store random numbers
	while( true )
	{
		randNum = rand() % RANDMAX;		
		if( randNum < (RANDMAX * 0.4) )				//40% probability checking mechanism
		{
			if( liveZombies > 0 )				//Check if any zombies are present
			{	
				killZombie();				//Kill a zombie
				killCount++;				//Increment kill count
			}
			else
				printf( "No zombies to kill\n" );
		}

		//Waiting for 10ms

		usleep(10000);						//Sleep for 10ms
		counter++;						

		//When 2s are elapsed

		if( counter%200 == 0 )					//For every 200 sleeps of 10ms length, ie, for every 2 seconds
		{
			if( liveZombies > threshold )
			{
				for( int i = 0; i < DOORS; i++ )
					activateDoors[i] = false;
			}
			printf( "2 seconds checkpoint\n" );
			if( liveZombies <= threshold/2 )		//If livezombies are less than half of threshold
			{
				printf( "Live zombies less than threshold / 2\n" );
				for( int i=0; i < DOORS ; i++ )		//Signal every waiting thread to let in more zombies
				{
					
					if( waiting[i] && !activateDoors[i])
					{
						printf( "Signalling door %d to let in more zombies\n", i+1);
						activateDoors[i] = true;
						pthread_cond_signal( &letInMore[i] );
						pthread_mutex_unlock( &letInMutex[i] );
					}
				}
			}
			printf( "\t\t\t\t\t\tThroughput : %d kills per second\n", killCount/(counter/100) );
		}
	}

	pthread_exit( NULL );
}

//FriendThread
void *friendThread( void *id )
{
	long zombiesAdmitted = 0;
	long myID = ( long )id;
	int randNum;
	activateDoors[myID-1] = true;
	while( true )
	{
		//Check if there are too many zombies

		if( !activateDoors[myID - 1] )
		{
			//If there are too many zombies, wait for signal from hero thread to open doors again.

			printf( "Thread %d is waiting for signal to let in more zombies\n", myID );
			pthread_mutex_lock ( &letInMutex[myID-1] );	
			waiting[myID-1] = true;
			pthread_cond_wait( &letInMore[myID-1], &letInMutex[myID-1] );
			
			printf( "Thread %d is resuming operation\n", myID );
			waiting[myID-1] = false;
		}
		randNum = rand() % RANDMAX; 

		if( randNum < (RANDMAX * 0.1) ) 
	 	{
	      		letInZombie();					//Let in one zombie
			zombiesAdmitted++;				//Incrementing the local count of zombies admitted inside
			printf( "Thread %d has let in a zombie\n", myID );
	    	}
		usleep(10000);						//Sleep for 10ms
	}

	pthread_exit( NULL );
}

int main( int argc, char *argv[] )
{
	//Getting threshold value as command line argument

	if( argc ==1 )							
	{
		printf( "You did not pass threshold as an argument\nSetting 20 as threshold and continuing...\n" );
		threshold = DEFAULTTHRESHOLD;
	}
	else if( argc > 2 )
	{
		char op;
		printf( "You have specified more than one arguments. Take the first argument as threshold and proceed ( Y / N ) ??\n" );
		cin >> op;
		if( op != 'y' && op != 'Y' )
		{
			printf( "Exiting the program ... \n" );
			exit( 0 );
		}
	}
	else
		threshold = (long)atoi( argv[1] );
	
	//Creating hero thread
	
	int hc = pthread_create( &hero, NULL, heroThread, (void *)threshold );
	if( hc )
	{
		cout<<"Error creating the hero thread. Code returned - "<<hc<<endl;
		exit(0);
	}
	else
		printf( "Hero thread created successfully\n" );

	int fc;
	long id;

	//Creating friend threads

	for( int i=0; i < DOORS; i++ )
	{
		id = i + 1;
	  	fc = pthread_create( &frnd[i], NULL, friendThread, (void *)id );
		if( fc )
		{
			cout<<"Error creating friend thread no "<<i+1<<". Code returned - "<<fc<<endl;
			exit(0);
		}
		else
			printf( "Friend thread %d created successfully\n", i+1 );
	}

	pthread_exit( NULL );
	return 0;
}
