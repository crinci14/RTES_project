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

	bool is_volatile();
	bool is_full(); //per gli inserimenti
	

private:

	size_t dim;
	bool transient_local;
	int time;
	int num_thread;
	sem_t full;
	sem_t mutex;

};
#endif