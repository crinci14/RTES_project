#ifndef QUEUE_MEX_H
#define QUEUE_MEX_H

#include<pthread.h>
#include<semaphore.h>

using namespace std;

//Definizione macro 

//dimensione della coda
#ifndef DIMENSIONE
#define DIMEMSIONE 100
#endif

#ifndef TRANSIENT_LOCAL
#define TRANSIENT_LOCAL true
#endif

#ifndef VOLATILE
#define VOLATILE false
#endif

//usiamo template per poter istanziare la struct con più tipi di dato
template <typename T>
struct elemento{
	T messaggio;
	chrono::steady_clock::time_point time;
};

template<class T> class Queue_mex{
public:

	Queue_mex(size_t dim = DIMENSIONE,int num_thread = 10, bool transient_local = TRANSIENT_LOCAL, int time = 100 );
	~Queue_mex();

	struct elemento pop_mex(int tid);// non bloccante anche se il thread non ha elementi da prendere in coda
	void push_mex(struct elemento message);// bloccante se la coda è piena

	bool transient_local();
	bool is_full(); //per gli inserimenti
	

private:

	size_t dim;
	bool transient_local;
	int time;
	int num_thread;// threads totali
	int active_threads;//threads attivi al momento

	struct elemento<T> *queue_mex;
	int head; // ci dice, quando è possibile, dove inserire  il nuovo messaggio nella coda
	int **taken_mex; // matrice che indica i messaggi presi (matrice thread/messaggio)
	int *next_pop;//indica, per ogni threads, qual è il prossimo messaggio da prendere nella coda
	sem_t *empty;//un semaforo per ogni thread, ci dice se per quel thread c'è o no un messaggio da prendere
	sem_t full;// ci si bloccano i thread che effettuano la push e non c'è spazio in coda 
	sem_t mutex;// semaforo di mutua esclusione nell'accesso alla coda e a variabili usate da tutti

};
#endif