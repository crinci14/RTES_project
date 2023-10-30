#include <iostream>
#include <stdlib.h>
#include "Queue_mex.h"
#include <unistd.h>
#include <chrono>

using namespace std;

// Costruttore
template<class T>
Queue_mex<T>::Queue_mex(size_t dim, int num_thread, bool transient_local, int time){
	if (dim <= 0)
	{
		cout << "Dimensione coda non accettata(minimo 1)." << endl;
		return 1;
	}
	if (levels <= 1)
	{
		cout << "Numero Thread non accettata(minimo 2)." << endl;
		return 1;
	}

	if (time <= 0)
	{
		cout << "Tempo negativo non accettato." << endl;
		return 1;
	}

	this->dim = dim;
	this->num_threads = num_threads;
	this->transient_local = transient_local;
	this->time = time;

	this->queue_mex = (struct element<T>*)malloc(this->dim*sizeof(struct element<T>)); // coda di strutture
	this->head = 0;
	this->tail = 0;
	this->empty = (sem_t*)malloc(this->num_threads*sizeof(sem_t));
	//allocazione dinamica della matrice
	this->taken_mex = (int**)malloc(this->num_threads*sizeof(int*));
	for (int i = 0; i < this->num_threads; i++)
		this->taken_mex[i] = (int*)malloc(this->dim*sizeof(int));

	/* verranno comunque sempre sovrascritti i dati prima di essere letti
	for (int i = 0; i < this->dim; i++)
	{
		queue_mex[i].messaggio = -1;
		queue_mex[i].time = chrono::steady_clock::now(); 
	}
	*/

	for (int i = 0; i < this->num_threads; i++)
	{
		for (int j = 0; i < this->dim; i++)
			this->taken_mex[i][j] = 0; 
	}

	//inizializzo semafori

	sem_init(&this->full, 0, this->dim);
	sem_init(&this->mutex, 0, 1);
	for (int i = 0; i < num_threads; i++)
		sem_init(&this->empty[i], 0, 0);


}