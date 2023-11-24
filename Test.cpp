#include<iostream>
#include<string>
#include<stdlib.h>
#include<unistd.h>

#include"Queue_mex.h"
#include"Queue_mex.cpp" 

using namespace std;

#define NUM_THREADS 10
#define DIM 20
#define TIME 10

////variabile globale
/////////////////////dim,  num_th,      modalità,     time
Queue_mex<int> queue(DIM, NUM_THREADS, TRANSIENT_LOCAL, TIME);

void pausetta()
{
	struct timespec t;
	t.tv_sec = 0;
	t.tv_nsec = (rand() % 10 + 1) * 1000000;
	nanosleep(&t, NULL);
}

void* client(void* arg)
{
	int controllo;// ci dice se la pop ha ricevuto un messaggio o non c'era nulla in coda
	int tid = queue.init_th();// mi registro,ora sono pronto per ricevere e inserire messaggi
	struct element<int> mex;
	int lap = 50;
	while (lap)
	{
		//faccio pop finchè ho messaggi da prendere, al primo vuoto mi fermo
		do {
			mex = queue.pop_mex(tid, controllo);
			if (controllo != -1)
			{
				cout << tid << "-> letto messaggio: " << mex.message << endl;
			}
			else
			{
				cout << tid << "-> non ci sono messaggi da leggere" << endl;// cout utile per il debug
			}
		} while (controllo != -1);

		if ((rand() % 10) < 5)
		{
			mex.time = chrono::steady_clock::now();
			mex.message = rand() % 100;
			queue.push_mex(mex, tid, controllo);
			if (controllo != -1)
			{
				cout << tid << "-> ho inserito un messaggio: " << mex.message << endl;//out utile per il debug
			}
			else
			{
				cout << tid << "-> messaggio non inserito, coda piena  " << endl;
			}
		}

		lap--;
		pausetta();
	}
	queue.termination_th(tid);//non si dovrà più aspettare che questo threads riceva i messaggi
	cout << tid << "-> terminato" << endl;

	return NULL;
}

int main()
{
	pthread_attr_t a;
	pthread_t *p;

	p = (pthread_t *)malloc(NUM_THREADS*sizeof(pthread_t));
	srand(time(NULL));
	pthread_attr_init(&a);

	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_create(&p[i], &a, client, NULL);
		pausetta();
	}

	pthread_attr_destroy(&a);

	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(p[i], NULL);
	}

	cout << "Tutti i threads sono terminati" << endl;
	free(p);
	return 0;
}





