#include<iostream>
#include<string>
#include<stdlib.h>
#include<unistd.h>

#include"Queue_mex.h"
#include"Queue_mex.cpp" 

using namespace std;

#define TYPE int
#define NUM_THREADS 10
#define DIM 20
#define TIME 10
#define LAP 50

//variabile globale
//-------------------dim,   num_th,     modalità,   time

Queue_mex<TYPE> queue(DIM, NUM_THREADS, TRANSIENT_LOCAL, 10);
//Queue_mex<int> queue;//costruttore con i parametri preimpostati
//Queue_mex<int> queue(100);

void pausetta()
{
	struct timespec t;
	t.tv_sec = 0;
	t.tv_nsec = (rand() % 10 + 1) * 1000000;// da 1 a 10 ms di pausa
	nanosleep(&t, NULL);
}

void* client(void* arg)
{
	int controllo;// ci dice se la pop/push ha ricevuto/inserito un messaggio
	int tid = queue.init_th();// mi registro e ricevo un identificativo, ora sono pronto per ricevere e inserire messaggi
	struct element<TYPE> mex;
	for (int i = 0; i < LAP ;i++)
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

		if ((rand() % 10) < 5)// con una certa probabilità inserisco il messaggio o no
		{
			//creo il messaggio
			mex.time = chrono::steady_clock::now();
			mex.message = rand() % 100;//messaggio inizializzato con un int
			//mex.message='a' + rand() % 26;// messaggio inizializzato con un char

			//inserisco il messaggio creato
			queue.push_mex(mex, tid, controllo);
			if (controllo != -1)
			{
				cout << tid << "-> ho inserito un messaggio: " << mex.message << endl;//cout utile per il debug
			}
			else
			{
				cout << tid << "-> messaggio non inserito, coda piena  " << endl;
			}
		}
		pausetta();
	}
	queue.termination_th(tid);//non si dovrà più aspettare che questo threads riceva i messaggi
	cout << tid << "-> terminato" << endl;//cout utile per il debug

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

	cout << "Tutti i threads sono terminati" << endl;//cout utile per il debug
	free(p);
	return 0;
}