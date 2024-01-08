# PROGETTO DI SISTEMI EMBEDDED E REAL-TIME

## Sommario
Questo progetto cerca di creare una libreria (file .cpp e .h) che possa essere usata per implementare una coda di strutture (a sua volta composte da messaggio e orario di invio) scambiate da più pthreads, si implementa la QoS della Durability.
Si posso impostare 2 modalità operative:
* Transient_local in cui quando un messaggio viene inviato ha il diritto di riceverlo qualsiasi thread che sia già attivo al momento oppure che arriva(si unisce al gruppo dei threads) entro un tempo limite che si puo impostare con una variabile.
* Volatile in cui quando un messaggio viene inviato ha il diritto di riceverlo qualsiasi thread che sia già attivo al momento e nessun altro che arrivi dopo.

Ogni thread ha un tempo indefinito per leggere il messaggio e solo quando tutti quelli che ne hanno il diritto lo hanno ricevuto questo verrà rimosso dalla coda per liberare spazio.

## Utilizzo della libreria

### Dichiarazione della classe
Si deve creare un istanza globale della classe.
In fase di dichiarazione bisogna specificare il tipo di messaggi che i threads si scambiano (TYPE) ed è possibile anche indicare i parametri costruttivi della classe(dimensione della coda, numero di threads, modalità operativa, tempo limite(ms)).
Questo si può fare in vari modi:
* Queue_mex<TYPE> queue; -> verrà chiamato il costruttore della classe con i parametri preimpostati(50,10,transient_local,10).
* Queue_mex<TYPE> queue(dimensione, numero threads, modalità operativa, tempo); -> verrà chiamato il costruttore della classe con i parametri specificati.
* Queue_mex<TYPE> queue(dimensione); -> verrà chiamato il costruttore con i parametri specificati e con i parametri mancanti verrà utilizzato il valore di default. Nota bene: non è possibile specificare il secondo parametro senza prima aver specificato il primo e cosi via.

La dimensione della coda deve essere impostata almeno al doppio del numero dei threads.
Quando la modalità operativa è impostata su "transient_local" il tempo limite non potrà essere negativo.
Quando la modalità operativa è impostata su "volatile" il tempo limite verrà ignorato.

### Utilizzo delle 4 funzioni principali

* int init_th(); -> non ha parametri e serve per registrare l'arrivo dei thread per poter poi inviare e ricevere messaggi, se ci sono messaggi inviati in precedeza che il thread ha il diritto di leggere modifica delle variabili. Ritorna un intero che indica l'id del thread.
* struct element<TYPE> pop_mex(int tid, int &controllo); -> ha come parametri di input l'id del thread e una variabile di controllo che ci indicherà se la procedura è andata a buon fine o meno. La funzione controlla se c'è un messaggio disponibile e in caso affermativo lo prende e se possibile elimina il messaggio dalla coda. Ritorna la struttura con il messaggio e il tempo d'invio.  
* void push_mex(struct element<TYPE> mex, int tid, int &controllo); -> ha come parametri di input la struttura da inserire in coda, l'id del thread e una variabile di controllo che ci indicherà se la procedura è andata a buon fine o meno. La funzione controlla inanzitutto se ci sono dei messaggi che è possibile liberare dalla coda e in caso li elimina, solo dopo si assicura che ci sia spazio in coda per inserire la nuova struttura.
* void termination_th(int tid); -> ha come parametri di input l'id del thread che sta per terminare. La funzione imposta come letti i messaggi non ancora ricevuti dal thread e in caso toglie dalla coda i messaggi che hanno già ricevuto tutti i threads. Da questo momento in poi il thread non avrà più il diritto di leggere i messaggi successivi. 

Nota bene: nessuna delle primitive è bloccante in quando ciascun threads può sia leggere che scrivere messaggi e renderle bloccanti potrebbe causare l'arresto di una delle 2 fasi. Per utilizzare al meglio la pop ed evitare riempimenti di coda, è bene che ad ogni ciclo di tempo si ricevano tutti i messaggi disponibili per quel thread.

## Variabili utilizzate interne alla classe

* size_t dim; -> indica la dimensione della coda.
* bool transient_local; -> indica se la modalità operativa è transient_local(true) o volatile(false).
* int time; -> indica il tempo limite in caso di modlità transient_local.
* int num_threads; -> indica i threads totali attivabili.
* int activated_threads; ->indica il numero totale dei threads attivi al momento o in passato.
* struct element<T> *queue_mex; -> indica il vettore di tutte le strutture contenenti i messaggi.
* int num_mex; -> indica il numero di messaggi presenti attualmente nella coda.
* chrono::steady_clock::time_point *arrivals; ->indica il vettore con il tempo di arrivo di ogni thread.
* int head; -> indica, quando è possibile, dove inserire  il nuovo messaggio nella coda.
* int tail; -> indica il primo dei messaggi inseriti che è ancora in coda, ed è l'unico che può essere tolto dalla coda.
* bool **taken_mex; -> indica la matrice dei messaggi presi (matrice thread/messaggio).
* int *next_pop; -> indica il vettore che ci dice, per ogni threads, qual è il prossimo messaggio da prendere nella coda.
* int num_mex_wait; -> indica quanti messaggi sono presi da tutti ma non è ancora scaduto il tempo di late join e quindi non possono essere eliminati e devono stare in attesa.
* bool *vett_mex_wait; -> indica il vettore collegato a num_mex_wait e ci dice quali messaggi sono in questa situazione.
* bool *finished_threads; -> indica il vettore dei threads che sono già terminati o meno.
* int num_tail_wait; -> ci dice quanti messaggi sono pronti per essere tolti dalla coda ma non sono il primo messaggio indicato da tail (perchè solo la tail può essere rimossa dalla coda).
* bool *vett_tail_wait; -> indica il vettore collegato a num_tail_wait e ci dice quali messaggi sono in questa situazione.
* sem_t *empty; -> indica un vettore di semafori, uno per ogni thread, ci dice se per quel thread c'è o no un messaggio da prendere (è utilizzato con la primitiva "try_wait" per non permettere di bloccarsi su questo semaforo).
* sem_t mutex; -> indica un semaforo di mutua esclusione nell'accesso alla coda e alle variabili usate da tutti.

## Come compilare ed eseguire il programma di Test della libreria

1. Compilare il file di Test
```
$ g++ Test.cpp -lpthread -o out
```
2. Eseguire il file appena creato
```
$ ./out
```