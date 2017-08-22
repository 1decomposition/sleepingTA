#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>

/* mutex declarations */
pthread_mutex_t mutex_lock;	/* protects global variable waiting_student */

/* semaphore declarations */
sem_t student_sem;	/* ta waits for a student to show up, student notifies ta his/her arrival */
sem_t ta_sem;	/* student waits for ta to help, ta notifies student he/she is ready to help */

/* the number of waiting students */
int waiting_students;

/* maximum time (in seconds) to sleep */
#define MAX_SLEEP_TIME	3

/* number of potential students */
#define NUM_OF_STUDENTS	42

/* number of times a student can receive hepl */
#define NUM_OF_HELPS	2

/* number of available seats */
#define NUM_OF_SEATS	2

/* student programs for a random time before asking tutor for help */
void programming(int id)
{
	unsigned int seed = rand();
	int time = (rand_r(&seed) % 3) + 1;
	printf("\tStudent %d programming for %d seconds\n", id, time);
	sleep(time);
}

/* student is currently receiving help from TA */
void tutoring_student()
{
	unsigned int seed = rand();
	int time = (rand_r(&seed) % 3) + 1;
	printf("Helping a student for %d seconds, # of waiting students = %d\n", time,waiting_students);
	sleep(time);
}

void* student_routine(void* id)
{
	int help_count = NUM_OF_HELPS;
	int ta_available = 0;
	int* std_id = (int*)id;
	while(help_count != 0)
	{
		programming(*std_id);
		/* signal TA for help */
		sem_post(&student_sem);
		ta_available = sem_trywait(&ta_sem);
		/* if TA is currently helping a student, take a seat*/
		if(ta_available != EAGAIN)
		{
			if(waiting_students < NUM_OF_SEATS)
			{
				waiting_students++;
				printf("\t\tStudent %d takes a seat, # of waiting students = %d\n", *std_id, waiting_students);
				/* student is receiving help */
				pthread_mutex_lock(&mutex_lock);
				waiting_students--;
				printf("Student %d receiving help\n", *std_id);
				sem_wait(&ta_sem);
				pthread_mutex_unlock(&mutex_lock);
				/* student done recieveing help, next student can take a seat, and one can get help */
				help_count--;
			}
			else
				/* if all seats are taken, student goes back to programming and takes a seat */
				printf("\t\t\tStudent %d will try again later\n", *std_id);
		}
		else
			help_count--;
	}
}

void* ta_routine(void* id)
{
	while(1)
	{
		/* waiting for student */
		sem_wait(&student_sem);

		/*helping student */
		sem_post(&ta_sem);
		tutoring_student();

		/* helping any waiting students */
		while(waiting_students > 0)
		{
			sem_post(&ta_sem);
			tutoring_student();
		}
	}
}

int main(void)
{
	pthread_t thread_ta;
	pthread_t* students;
	int* student_id;
	waiting_students = 0;
	srand(time(NULL));
	int i = 0;

	printf("%s", "CS149 SleepingTA from Justin Coy\n");

	/* allocate memory for threads & thread ids */
	students = (pthread_t*) malloc(sizeof(pthread_t) * NUM_OF_STUDENTS);
	student_id = (int*) malloc(sizeof(int) * NUM_OF_STUDENTS);

	/* threads are assigned [1,..., # students] */
	while(i < NUM_OF_STUDENTS)
	{
		student_id[i] = i + 1;
		i++;
	}
	i = 0;

	/* initialize mutex, and semaphores */
	pthread_mutex_init(&mutex_lock, NULL);
	sem_init(&student_sem, 0, 0);
	sem_init(&ta_sem, 0, 0);

	/* create threads */
	pthread_create(&thread_ta, NULL, ta_routine, NULL);	
	while(i < NUM_OF_STUDENTS)
	{
		pthread_create(&students[i], NULL, student_routine, (void*)&student_id[i]);
		i++;
	}
	i = 0;

	/* wait for threads to finish */
	while(i < NUM_OF_STUDENTS)
	{
		pthread_join(students[i], NULL);
		i++;
	}

	/* when student threads finish, ta leaves office */
	pthread_cancel(thread_ta);
	free(students);
	free(student_id);

	return(0); 
}
