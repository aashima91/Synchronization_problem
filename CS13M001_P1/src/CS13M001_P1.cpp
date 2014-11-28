/*
 * main.cpp
 *
 *  Created on: Mar 5, 2014
 *     
 */

#include<iostream>
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<unistd.h>
#define MAXSTUDENTS 25
using namespace std;

pthread_mutex_t answerMutex;
pthread_mutex_t questionMutex, completed;
pthread_mutex_t printLock;
pthread_cond_t qnComplete;
pthread_cond_t answerComplete;
bool waiting = false, waiting2 = false, finished = false;

int studentNo, questionNo, Sno, QperStudent;
int Entering[MAXSTUDENTS];
int Number[MAXSTUDENTS];
long qnsAnswered;

int maxToken()
{
	int max=Number[0];
	for( int i = 0; i < MAXSTUDENTS; i++ )
	{
		if( Number[i] > max )
			max = Number[i];
	}
	return max;
}

void lock( int i )
{
	Entering[i] = 1;
	Number[i] = 1 + maxToken();
	Entering[i] = 0;
	for( int j = 0; j < MAXSTUDENTS; j++ )
	{
		while( Entering[j] )	
		{}
		while( (Number[j] != 0) && ( Number[j] < Number[i] ) ) 
		{}
	}
}

void unlock( int i )
{
	Number[i] = 0;
}

void AnswerStart( )
{
	//printf( "Inside AnswerStart()\n" );

	pthread_mutex_lock( &answerMutex );
	waiting2 = true;
	pthread_cond_wait ( &qnComplete, &answerMutex );
	waiting2 = false;
	
	sched_yield();

}

void AnswerDone( )
{
	//printf( "Inside AnswerDone()\n" );
	printf( "Professor finished answering Question no %d from Student %d\n", questionNo, studentNo );
	while( waiting == false )
	{ }
	pthread_cond_signal( &answerComplete );
	pthread_mutex_unlock( &answerMutex );

	sched_yield();
}

void QuestionStart( long id, long q )
{
	//printf( "Inside QuestionStart()\n" );
	printf( "Student %d is ready to ask a question\n", id );


	pthread_mutex_lock( &questionMutex );
	printf( "Student %d is asking question no %d\n", id, q );
	usleep(10000);
	studentNo = id;
	questionNo = q;
	while( waiting2 == false )
	{ }
	pthread_cond_signal( &qnComplete );

	sched_yield();

}

void QuestionDone( int id, int q )
{
	//printf( "Inside QuestionDone()\n" );

	pthread_mutex_lock( &completed );
	waiting = true;
	pthread_cond_wait( &answerComplete, &completed );
	waiting = false;
	
	printf( "Student %d finished with question no %d\n", id, q );
	
	pthread_mutex_unlock( &completed );
	pthread_mutex_unlock( &questionMutex );
	finished = true;
	sched_yield();
}

void *profThread( void *id )
{
	long profId;
	qnsAnswered = 0;
	profId = (long)id;
	int num;
	
	while(true)
	{	finished = false;
		if( qnsAnswered == Sno * QperStudent )
			break;
		else
			qnsAnswered++;
		printf( "Professor is ready to answer\n" );	
		AnswerStart();
		num = rand()%14 + 2;
		//nanosleep((struct timespec[]){{0, num*1000000000}}, NULL);
		usleep( num * 10000 );
		printf( "Professor is answering Question no %d from Student %d\n", questionNo, studentNo );
		
		AnswerDone();
		while( finished == false )
		{ }
	}

	printf( "All questions answered. \n\tSTATS : %d questions from %d students answered. \n\tProfessor is calling it a day... \n\n", qnsAnswered, Sno );

	pthread_exit( NULL );
}

void *studentThread( void *id )
{
	long myID;
	long qno = 1;
	myID = (long)id;

	while( qno <= QperStudent )
	{
		lock( myID-1 );

		QuestionStart( myID, qno );

		//printf( "Student %d is asking question no %d\n", id, qno );
		
		QuestionDone( myID, qno );
		qno++;

		unlock( myID-1 );
	}

	pthread_exit( NULL );
}

int main( int argc, char *argv[] )
{
	if( argc == 1 )
	{
		Sno = MAXSTUDENTS ;
		QperStudent = 10 ;
	}
	else if ( argc == 2 )
	{
		Sno = atoi( argv[1] );
		QperStudent = 10;
	}
	else
	{
		Sno = atoi( argv[1] );
		QperStudent = atoi( argv[2] );
	}

	pthread_t prof;
	pthread_t student[MAXSTUDENTS];

	int pc,sc;

	//Creating Professor thread
	printf( "Creating Professor thread\n" );
	pc = pthread_create( &prof, NULL, profThread, (void *)1 );
	if( pc )
	{
		printf( "Error creating Professor thread. Code returned - %d\n" );
		exit(0);
	}

	//Creating Student threads
	for(long i = 0; i < Sno; i++ )
	{
		long k = i+1;
		sc = pthread_create ( &student[i], NULL, studentThread, (void *) k );
		if( sc )
		{
			printf( "Error creating student thread %d. Code returned - %d\n", i, sc );
			exit(0);
		}
		/*else
		{
			pthread_mutex_trylock( &printLock );
			cout<<"Student thread "<<i+1<<" created"<<endl;
			pthread_mutex_unlock( &printLock );
		}*/
	}

	//Printing thread creation complete message

	printf( "All threads created. 1 - Prof & %d - Students\n", MAXSTUDENTS );

	pthread_exit( NULL );

}

