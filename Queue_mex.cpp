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
		return;
	}
	if (num_threads <= 1)
	{
		cout << "Numero Thread non accettato(minimo 2)." << endl;
		return;
	}

	if (time <= 0)
	{
		cout << "Tempo negativo non accettato." << endl;
		return;
	}

	this->dim = dim;
	this->num_threads = num_threads;
	this->transient_local = transient_local;
	if (this->transient_local)
		this->time = time;
	else
		this->time = 0;

	this->active_threads = 0; 
	this->queue_mex = (struct element<T>*)malloc(this->dim*sizeof(struct element<T>)); // coda di strutture
	this->arrivals = (chrono::steady_clock::time_point*)malloc(this->num_threads*sizeof(chrono::steady_clock::time_point));
	this->next_pop = (int*)malloc(this->num_threads*sizeof(int));
	this->head = 0;
	this->tail = 0;
	this->empty = (sem_t*)malloc(this->num_threads*sizeof(sem_t));
	//allocazione dinamica della matrice
	this->taken_mex = (bool**)malloc(this->num_threads*sizeof(bool*));		

	for (int i = 0; i < this->num_threads; i++)
	{
		this->taken_mex[i] = (bool*)malloc(this->dim*sizeof(bool));
		this->next_pop[i] = 0;
		for (int j = 0; j < this->dim; j++)
			this->taken_mex[i][j] = false; 
	}

	this->mex_wait = false;
	this->num_mex_wait = 0;
	this->vett_mex_wait = (int*)malloc(this->dim*sizeof(int));
	for (int i = 0; i < this->dim; i++)
		this->vett_mex_wait[i] = 0;
		

	//inizializzo semafori

	sem_init(&this->full, 0, this->dim);
	sem_init(&this->mutex, 0, 1);
	for (int i = 0; i < num_threads; i++)
		sem_init(&this->empty[i], 0, 0);


}


//Distruttore

template<class T>
Queue_mex<T>::~Queue_mex(){}



template<class T>
int Queue_mex<T>::init_th()
{
	sem_wait(&this->mutex);
	int tid = this->active_threads;
	this->active_threads++;
	this->arrivals[tid] = chrono::steady_clock::now();//segno l'ora di join del thread
	
	// nel momento dell'inserimento del messaggio viene inserito come già preso da parte di un thread non ancora arrivato
	// e se sono ancora in tempo questo va modificato
	for (int i = 0; i < this->num_mex; i++)
	{
		int index = (this->tail + i) % this->dim;
		const auto diff = chrono::duration_cast<chrono::milliseconds>(this->arrivals[tid] - this->queue_mex[index].time).count();
		
		if (diff <= this->time)
		{
			if (this->vett_mex_wait[index] == 1)
			{// modifico le variabili
				this->num_mex_wait--;
				this->vett_mex_wait[index] = 0;
			}
			this->taken_mex[tid][index] = false;
			sem_post(&this->empty[tid]);
		}
	}

	sem_post(&this->mutex);

	return tid;
}


//dato il messaggio all'interno della coda ci dice se tutti i thread che ne hanno diritto lo hanno ricevuto
// se il vettore è di tutti 1 allora lo hanno preso tutti i threads che ne avevano il diritto
template<class T>
bool Queue_mex<T>::taken_all(int mex)
{
	bool finish = true;
	
	for (int i = 0; i < this->num_threads; i++)
	{
		if (!this->taken_mex[i][mex])
			finish = false;
	}
		
	return finish;
}

// pop del messaggio parametrica del thread specifico
template<class T>
struct elemento Queue_mex<T>::pop_mex(int tid)
{
	//controllo se ci sono messaggi da leggere
	int i = sem_trywait(&this->empty[tid]);
	if (i == -1)
	{
		cout << tid << ": per il thread non ci sono messaggi da leggere" << endl;// cout utile per il debug
		return NULL;
	}

	//ho almeno un messaggo da prendere, quindi next_pop indicherà sempre il primo messaggio disponibile
	sem_wait(&this->mutex);
	
	struct elemento ret;
	ret = this->queue_mex[this->next_pop[tid]];
	this->taken_mex[tid][this->next_pop[tid]] = true;
	const auto diff = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - queue_mex[this->next_pop[tid]].time).count();
	if (taken_all(this->next_pop[tid]))
	{   
		//// imposto le variabili di controllo
		this->num_mex_wait++;
		this->vett_mex_wait[this->next_pop[tid]] = 1;
		if (this->active_threads == this->num_threads || diff > this->time)
		{
			//il più vecchio messaggio che è ancora in coda è stato preso da tutti i thread ed è anche passato il tempo di late join o sono già arrivati tutti i threads.
			this->num_mex_wait--;
			this->vett_mex_wait[this->next_pop[tid]] = 0;

			this->num_mex--;
			this->tail = (this->tail + 1) % this->dim;
			sem_post(&this->full);
		}
	}
	this->next_pop[tid] = (this->next_pop[tid] + 1) % this->dim;

	sem_post(&this->mutex);

	return ret;
}

////inserimento messaggio nella coda, è bloccante con coda piena
template<class T>
void Queue_mex<T>::push_mex(struct elemento message, int tid)
{
	sem_wait(&this->mutex);

	if (this->num_mex_wait > 0)
	{///// ci potrebbero essere dei messaggi vecchi da cancellare che possono sbloccare delle push

		chrono::steady_clock::time_point now = chrono::steady_clock::now();
		int tail = this->tail;
		int num_mex_wait = this->num_mex_wait;
		for (int i = 0; i < num_mex_wait; i++)
		{
			int index = (tail + i) % this->dim;
			const auto diff = chrono::duration_cast<chrono::milliseconds>(now - queue_mex[index].time).count();
			if (diff > this->time)
			{
				this->num_mex_wait--;
				this->vett_mex_wait[index] = 0;

				this->num_mex--;
				this->tail = (this->tail + 1) % this->dim;
				sem_post(&this->full);
			}

		}
	}

	// se mi devo bloccare devo rilasciare prima il mutex
	if (this->num_mex == this->dim)
	{
		sem_post(&this->mutex);
		sem_wait(&this->full);
		sem_wait(&this->mutex);
	}
	else sem_wait(this->full);

	this->queue_mex[this->head] = message;
	this->num_mex++;

	for (int i = 0; i < this->num_threads; i++)
	{
		if (i < this->active_threads && i != tid)
		{
			this->taken_mex[i][this->head] = false;
			sem_post(&this->empty[i]);
		}
		else
			this->taken_mex[i][this->head] = true;
	}
	this->head = (this->head + 1) % this->dim;

	sem_post(&this->mutex);
	return;
}