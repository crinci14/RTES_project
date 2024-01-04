#include <iostream>
#include <stdlib.h>
#include "Queue_mex.h"
#include <unistd.h>

using namespace std;

// Costruttore
template<class T>
Queue_mex<T>::Queue_mex(size_t dim, int num_threads, bool transient_local, int time)
{
	if (num_threads <= 1)
	{
		cout << "Numero Thread non accettato(minimo 2)." << endl;
		return;
	}
	
	if (dim < 2*num_threads)
	{
		cout << "Dimensione coda non accettata(minimo il doppio del numero dei threads)." << endl;
		return;
	}

	if (time < 0)
	{
		cout << "Tempo negativo non accettato." << endl;
		return;
	}
	this->cont=0;
	
	
	this->dim = dim;
	this->num_threads = num_threads;
	this->transient_local = transient_local;
	if (this->transient_local)
		this->time = time*1000000;// trasformo in nanosecondi
	else
		this->time = 0;

	this->activated_threads = 0; 
	this->queue_mex = (struct element<T>*)malloc(this->dim*sizeof(struct element<T>)); // coda di strutture
	this->arrivals = (chrono::steady_clock::time_point*)malloc(this->num_threads*sizeof(chrono::steady_clock::time_point));
	this->next_pop = (int*)malloc(this->num_threads*sizeof(int));
	this->head = 0;
	this->tail = 0;
	this->empty = (sem_t*)malloc(this->num_threads*sizeof(sem_t));
	this->finished_threads = (bool*)malloc(this->num_threads*sizeof(bool));
	//allocazione dinamica della matrice
	this->taken_mex = (bool**)malloc(this->num_threads*sizeof(bool*));		

	for (int i = 0; i < this->num_threads; i++)
	{
		this->finished_threads[i] = false;
		this->taken_mex[i] = (bool*)malloc(this->dim*sizeof(bool));
		this->next_pop[i] = 0;
		for (int j = 0; j < this->dim; j++)
			this->taken_mex[i][j] = false; 
	}
	
	this->num_tail_wait = 0;
	this->vett_tail_wait = (bool*)malloc(this->dim*sizeof(bool));
	for (int i = 0; i < this->dim; i++)
		this->vett_tail_wait[i] = false;
	
	

	this->num_mex_wait = 0;
	this->vett_mex_wait = (int*)malloc(this->dim*sizeof(int));
	for (int i = 0; i < this->dim; i++)
		this->vett_mex_wait[i] = 0;
		

	//inizializzo semafori

	sem_init(&this->mutex, 0, 1);
	for (int i = 0; i < num_threads; i++)
		sem_init(&this->empty[i], 0, 0);
}

//Distruttore
template<class T>
Queue_mex<T>::~Queue_mex()
{
	cout<< "Distruttore"<<endl;
	for (int i = 0; i < this->num_threads; i++)
		delete this->taken_mex[i];
	delete this->taken_mex;

	delete this->queue_mex;
	delete this->arrivals;
	delete this->next_pop;
	delete this->empty;
	delete this->vett_mex_wait;
	delete this->vett_tail_wait;

}

//segna l'arrivo del threads
template<class T>
int Queue_mex<T>::init_th()
{
	sem_wait(&this->mutex);
	int tid = this->activated_threads;
	this->activated_threads++;
	this->arrivals[tid] = chrono::steady_clock::now();//segno l'ora di join del thread
	this->next_pop[tid]=this->tail;//inizializzo la tail
	cout << tid <<"-> join" << endl;
	
	// nel momento dell'inserimento del messaggio viene inserito come gi� preso da parte di un thread non ancora arrivato
	// e se sono ancora in tempo questo va modificato
	for (int i = 0; i < this->num_mex; i++)
	{
		int index = (this->tail + i) % this->dim;
		const auto diff = chrono::duration_cast<chrono::nanoseconds>(this->arrivals[tid] - this->queue_mex[index].time).count();
		cout << diff<<" con mex " << i << endl;
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
// se il vettore � di tutti 1 allora lo hanno preso tutti i threads che ne avevano il diritto
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


//controlla se i messaggi possono essere tolti dalla coda, e quindi considerati presi da tutti gli aventi diritto
template<class T>
void Queue_mex<T>::check_mex(int tid)
{
	const auto diff = chrono::duration_cast<chrono::nanoseconds>(chrono::steady_clock::now() - this->queue_mex[this->next_pop[tid]].time).count();
	if (taken_all(this->next_pop[tid]))
	{   
		//// imposto le variabili di controllo
		this->num_mex_wait++;
		this->vett_mex_wait[this->next_pop[tid]] = 1;
		if (this->activated_threads == this->num_threads || diff > this->time)
		{
			//il pi� vecchio messaggio che � ancora in coda � stato preso da tutti i thread, ed � anche passato il tempo di late join o sono gi� arrivati tutti i threads.
			this->num_mex_wait--;
			this->vett_mex_wait[this->next_pop[tid]] = 0;
			this->num_tail_wait++;
			this->vett_tail_wait[this->next_pop[tid]]=true;
			
			if((this->tail) == (this->next_pop[tid])){
			
			this->num_tail_wait--;
			this->vett_tail_wait[this->next_pop[tid]]=false;
			
			
			this->num_mex--;
			
			cout<<this->cont<<"cont"<<endl;
			cout<< this->tail << " " << this->next_pop[tid] << endl;
			cout << tid << "-> Messaggio liberato (" << this->queue_mex[this->tail].message << ") dalla coda, ora sono presenti " << this->num_mex << " messaggi" << endl;// utile per debug
			cout << tid << "-> Messaggio liberato (" << this->queue_mex[this->next_pop[tid]].message << ") dalla coda, ora sono presenti " << this->num_mex << " messaggi" << endl;
			
			this->tail = (this->tail + 1) % this->dim;
			}
			
		}
	}	
}

// pop del messaggio parametrica del thread specifico
template<class T>
struct element<T> Queue_mex<T>::pop_mex(int tid, int &controllo)
{
	sem_wait(&this->mutex);
	controllo = 0;
	struct element<T> ret;
	//controllo se ci sono messaggi da leggere
	int i = sem_trywait(&this->empty[tid]);//da coprire con mutex
	if (i == -1)
	{
		controllo = -1;
		sem_post(&this->mutex);
		return ret;
	}

	//ho almeno un messaggo da prendere, devo prima controllare che next_pop non indichi ad un messaggio inserito da me o troppo vecchio
	//sem_wait(&this->mutex);
	
	//cerco il prossimo messaggio da prendere
	while(this->taken_mex[tid][this->next_pop[tid]] == true)
	{
		this->next_pop[tid] = (this->next_pop[tid] + 1) % this->dim;
	}
	
	ret = this->queue_mex[this->next_pop[tid]];
	this->taken_mex[tid][this->next_pop[tid]] = true;
	
	//controllo se si pu� togliere il messaggio
	check_mex(tid);

	//this->next_pop[tid] = (this->next_pop[tid] + 1) % this->dim;

	sem_post(&this->mutex);

	return ret;
}

template<class T>
bool Queue_mex<T>::is_full()
{
	return (this->num_mex == this->dim);
}

////inserimento messaggio nella coda, � bloccante con coda piena
template<class T>
void Queue_mex<T>::push_mex(struct element<T> mex, int tid, int &controllo)
{
	controllo = 0;
	
	sem_wait(&this->mutex);

	if (this->num_mex_wait > 0)
	{///// ci potrebbero essere dei messaggi vecchi da cancellare che possono sbloccare delle push

		chrono::steady_clock::time_point now = chrono::steady_clock::now();
		//int tail = this->tail;
		int index= this->tail;
		int num_mex_wait = this->num_mex_wait;
		for (int i = 0; i < num_mex_wait; i++)
		{
			while(!this->vett_mex_wait[index])
			{
				index = (index + 1) % this->dim;
			}
			
			const auto diff = chrono::duration_cast<chrono::nanoseconds>(now - this->queue_mex[index].time).count();
			if (diff > this->time)
			{
				cout<<this->vett_mex_wait[index]<< "vettore" <<endl;
				this->num_mex_wait--;
				this->vett_mex_wait[index] = 0;
				
				
				this->num_tail_wait++;
				this->vett_tail_wait[index]=true;
			
				if((this->tail) == (index)){
			
					this->num_tail_wait--;
					this->vett_tail_wait[index]=false;
			
					this->num_mex--;
					cout<< this->tail << " " << index << endl;
					cout << tid <<"-> Messaggio liberato (" << this->queue_mex[index].message << ") dalla coda in push, ora sono presenti " << this->num_mex << " messaggi" << endl;// utile per il debug
					cout << tid <<"-> Messaggio liberato (" << this->queue_mex[this->tail].message << ") dalla coda in push, ora sono presenti " << this->num_mex << " messaggi" << endl;
					
					this->tail = (this->tail + 1) % this->dim;
				}
			}

		}
	}
	
	
	for(int i=0;i<this->dim;i++)
		cout<<this->vett_tail_wait[i];
	cout<<endl;
	
	
	cout<< this->tail <<" " <<this->num_tail_wait<<endl;
	
	while((this->num_tail_wait > 0) && (this->vett_tail_wait[this->tail]))
	{
		
		this->num_tail_wait--;
		this->vett_tail_wait[this->tail]=false;
		this->num_mex--;
		cout << "ho usato tail wait, ora ci sono "<< this->num_mex << " messaggi" << endl;
		cout << "liberato messaggio " << this->queue_mex[this->tail].message  << endl;
		this->tail = (this->tail + 1) % this->dim;
			
	}
	
	for(int i=0;i<this->dim;i++)
		cout<<this->vett_tail_wait[i];
	cout<<endl;
	
	
	
	// se non c'� spazio in coda devo uscire dalla funzione
	if (is_full())
	{
		controllo=-1;
		sem_post(&this->mutex);
		return;
	}

	this->queue_mex[this->head] = mex;
	this->num_mex++;
	
	bool unico=true;//ci dice se chi inserisce il messaggio � l'unico threads attivo
	for (int i = 0; i < this->num_threads; i++)
	{
		// se non  mi trovo in uno di questi 3 casi segno il messaggio come gi� letto
		if ((i < this->activated_threads) && (i != tid) && (!(this->finished_threads[i])))
		{
			unico=false;
			this->taken_mex[i][this->head] = false;
			cout << tid<< "-> post per " << i << endl;
			sem_post(&this->empty[i]);
		}
		else
			this->taken_mex[i][this->head] = true;
	}
	
	if(unico)
	{
		this->cont++;
		this->num_mex_wait++;
		this->vett_mex_wait[this->head] = 1;
		
	}
	
	
	
	
	
	this->head = (this->head + 1) % this->dim;
	
	cout << tid <<  "-> Messaggio inserito in coda, ora sono presenti " << this->num_mex << " messaggi" << endl;// utile per il debug
	
	sem_post(&this->mutex);
	return;
}


template<class T>
void Queue_mex<T>::termination_th(int tid)
{
	sem_wait(&this->mutex);
	
	this->finished_threads[tid]=true;
	
	//se entro dentro vuol dire che ho dei messaggi non ancora letti e devo modificare le variabili prima di terminare
	while ((sem_trywait(&this->empty[tid])) != -1)
	{
		//cerco il prossimo messaggio che avrei dovuto prendere
		while(this->taken_mex[tid][this->next_pop[tid]] == true)
		{
			this->next_pop[tid] = (this->next_pop[tid] + 1) % this->dim;
		}

		this->taken_mex[tid][this->next_pop[tid]] = true;
		
		//controllo se si pu� togliere il messaggio
		check_mex(tid);
		//this->next_pop[tid] = (this->next_pop[tid] + 1) % this->dim;
	}
	
	
	sem_post(&this->mutex);
	
	return;
}

