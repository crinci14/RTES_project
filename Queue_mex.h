#ifndef QUEUE_MEX_H
#define QUEUE_MEX_H

#include<pthread.h>
#include<semaphore.h>
#include <chrono>

using namespace std;

//Definizione macro 

#ifndef TRANSIENT_LOCAL
#define TRANSIENT_LOCAL true
#endif

#ifndef VOLATILE
#define VOLATILE false
#endif

//usiamo template per poter istanziare la struct con più tipi di dato
template <typename T>
struct element{
	T message;
	chrono::steady_clock::time_point time;
};

template<class T> class Queue_mex{
public:

	//costruttore con parametri di default se non specificati
	Queue_mex(size_t dim = 50, int num_thread = 10, bool transient_local = TRANSIENT_LOCAL, int time = 10);
	~Queue_mex();

	int init_th();// segna il tempo di arrivo di ogni threads e aggiusta, se necessario, delle variabili
	struct element<T> pop_mex(int tid, int &controllo);// non bloccante anche se il thread non ha elementi da prendere in coda
	void push_mex(struct element<T> mex, int tid, int &controllo);//non bloccante anche se la coda è piena, il messaggio andrà scartato
	void termination_th(int tid);//quando i threads terminano non devono più essere aspettati per leggere i messaggi

private:
	//procedure usate internamente da altre funzioni
	bool is_full();// ci dice se la coda è piena
	bool taken_all(int mex);
	void check_mex(int tid);

	//variabili interne
	size_t dim;// dimensione della coda
	bool transient_local;// la modalità transient_local da il diritto a un thread che arriva dopo (in base a quanto dice time) all'invio del messaggio di poterlo leggere
	int time;// per il transient_local in millisecondi
	int num_threads;// threads totali
	int activated_threads;//threads attivi al momento o in passato

	struct element<T> *queue_mex;// vettore dei messaggi
	int num_mex;//messaggi presenti nella coda
	chrono::steady_clock::time_point *arrivals;// vettore con il tempo di arrivo di ogni thread
	int head; // ci dice, quando è possibile, dove inserire  il nuovo messaggio nella coda
	int tail;//primo dei messaggi inseriti che è ancora in coda, ed è l'unico che può essere tolto dalla coda
	bool **taken_mex; // matrice che indica i messaggi presi (matrice thread/messaggio)
	int *next_pop;//indica, per ogni threads, qual è il prossimo messaggio da prendere nella coda
	int num_mex_wait;//ci dice quanti messaggi sono presi da tutti ma non è ancora scaduto il tempo di late join
	bool *vett_mex_wait;//collegato a num_mex_wait ci dice quali messaggi sono in questa situazione
	bool *finished_threads;//vettore dei threads che sono già terminati e che quindi non verranno aspettati
	int num_tail_wait;// ci dice quanti messaggi sono pronti per essere tolti dalla coda ma non sono il primo messaggio indicato da tail
	bool *vett_tail_wait;//collegato a num_tail_wait ci dice quali messaggi sono in questa situazione

	sem_t *empty;//un semaforo per ogni thread, ci dice se per quel thread c'è o no un messaggio da prendere
	sem_t mutex;// semaforo di mutua esclusione nell'accesso alla coda e a variabili usate da tutti

};
#endif
