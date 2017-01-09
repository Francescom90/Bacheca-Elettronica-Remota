#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>


#define USER 16
#define PW 16
#define OGGETTO 32
#define TESTO 800
#define TIMEOUT 10


//Definizione dei tipi strutturati necessari
typedef struct {
	char user_name[USER];
	char user_pw[PW];
	} user_data;
	

typedef struct {
	char op;
	char user_name[USER];
	char user_pw[PW];
	char target_post;
	char mittente[USER];
	char oggetto[OGGETTO];
	char testo[TESTO];
	} richiesta;
	
	


int ds_socket;
//Funzioni per la gestione dei segnali
void gestione_alarm() {
	printf("Scaduto time-out prestabilito, la connessione con il server è stata interrotta. Terminazione\n");
	close(ds_socket);
	exit(0);
	}
void gestione_showdown() {
	printf("Il Client sta eseguendo lo showdown\n");
	close(ds_socket);
	exit(0);
	}
void gestione_chiusura(){
	printf("E' occorso un problema, il client verrà terminato\n");
	close(ds_socket);
	exit(0);
	}

//Funzione Main
void main(int argc,char* argv[]) {
	printf("Startup del client\n");
	//dichiarazione variabili locali
	char check;
	user_data utente;
	int result;
	int counter;
	richiesta req;
	sigset_t sset;	   //Maschera dei segnali da bloccare
	char buffer[1024]; 
	
	
	//gestione segnali. Imposto il comportamento che il client deve tenere se riceve uno specifico segnale
	signal(SIGHUP,  gestione_chiusura);
	signal(SIGINT,  gestione_chiusura);
	signal(SIGQUIT, gestione_chiusura);
	signal(SIGILL,  gestione_chiusura);
	signal(SIGSEGV, gestione_chiusura);
	signal(SIGTERM, gestione_showdown);
	signal(SIGALRM, gestione_alarm);
	signal(SIGCHLD, SIG_IGN);
	
	memset(&sset, 0, sizeof(sigset_t)); //Azzero la maschera dei segnali

	//Riempo il set di segnali e ci tolgo quelli di interesse
	sigfillset(&sset);
	sigdelset(&sset,SIGHUP);
	sigdelset(&sset,SIGINT);
	sigdelset(&sset,SIGQUIT);
	sigdelset(&sset,SIGILL);
	sigdelset(&sset,SIGSEGV);
	sigdelset(&sset,SIGTERM);
	sigdelset(&sset,SIGALRM);
	sigdelset(&sset,SIGCHLD);
	
	//aquisizione dati per l'autenticazione
	printf("Inserire user-name,numero massimo di caratteri 32 (non puù iniziare con #):\n");
	if(read_and_check(utente.user_name,USER)==-1){
		printf("Username non acquisito nella maniera corretta\n");
		raise(SIGILL);
		}
	printf("Inserire la propria password,numero massimo di caratteri 32\n");
	if(read_and_check(utente.user_pw,PW)==-1){
		printf("Password non acquisita nella maniera corretta\n");
		raise(SIGILL);
		}
	
	//creazione socket
	struct sockaddr_in server;
	struct hostent* ht;
	
	ds_socket=socket(AF_INET,SOCK_STREAM,0);
	if(ds_socket==-1){
		printf("Impossibile creare il socket\n");
		exit(0);
		}
	printf("Socket creato\n");
	//compilazione indirizzo
	memset((void*)&server,0,sizeof(server));
	server.sin_family=AF_INET;
	server.sin_port=htons(7777);
	ht=gethostbyname("localhost");
	memcpy(&server.sin_addr, ht->h_addr,4);
	
	//selezione del operazione e operazioni preliminari
	do {
		//azzero la richiesta da precedente contenuto informativo per una nuovo utilizzo
		memset((void*)&req,0,sizeof(req));
		strcpy(req.user_name,utente.user_name);
		strcpy(req.user_pw,utente.user_pw);
		do {
			//mando in stampa le istruzioni per l'utilizzo finché l'user non inserisce un operazione valida
			printf("\nBacheca elettronica: premere il tasto corrispondente all'operazione desiderata\n");
			printf("\t1- Leggere tutti i messaggi sulla bacheca elettronica\n");
			printf("\t2- Spedire un nuovo messaggio sulla bacheca elettronica\n");
			printf("\t3- Rimuovere un proprio messaggio dalla bacheca elettronica (eseguire prima il comando 1 per il listato dei messaggi)\n");
			printf("\t0- Chiudere la connessione con la bacheca elettronica\n");
			req.op=getchar();
			} while(req.op!='0'||req.op!='1'||req.op!='2'||req.op!='3');
		
		//in base all'operazione scelta eseguo delle istruzioni preliminari per compilare la richiesta per il server
		switch(req.op){
			case '1': //lettura dei messaggi della bacheca
				printf("Operazione selezionata: Visualizzazione bacheca\n");
				break;
					
			case '2': //Invio nuovo messaggio nella bacheca
				printf("Operazione selezionata: Invio messaggio\n\n");
				strcpy(req.mittente,req.user_name);
				printf("Inserire l'oggetto del messaggio[max 32 caratteri]\n");
				if(read_and_check(req.oggetto,OGGETTO)==-1){
					printf("Oggetto non acquisito nella maniera corretta\n");
					raise(SIGILL);
					}
				printf("Inserire il testo del messaggio[max 800 caratteri]\n");
				if(read_and_check(req.testo,TESTO)==-1){
					printf("Testo non acquisito nella maniera corretta\n");
					raise(SIGILL);
					}
					break;
					
			case '3': //rimozione messaggio da bacheca
				printf("Operazione selezionata: Eliminazione messaggio\n Indicare il numero del messaggio da eliminare\n");
				req.target_post=getchar();
				if(!isdigit(req.target_post))
					printf("Non è stato inserito un numero di messaggio valido \n");
					raise(SIGILL);
				break;
				
			case '4': //chiusura della applicazione
				printf("Chiusura richiesta dell'applicazione da parte dell'utente\n");
				raise(SIGTERM);
			}
	
	
	
		//tentativo di connessione
		if(connect(ds_socket,(struct sockaddr*)&server,sizeof(server))==-1){ //provo ad effetuare la connessione e controllo se è andata a buon fine
			printf("Impossibile stabilire la connessione\n");
			raise(SIGILL);
			}
		printf("Connessione stabilita\n");
		
		//autenticazione e invio richiesta
		if(delive(ds_socket,&req,sizeof(req))==-1){ //invio i dati per l'autenticazione e controllo la riuscita del invio
			printf("Impossibile inviare la richiesta\n");
			raise(SIGILL);
			}
		printf("Richiesta inviata\n");
	
		//risultato invio richiesta
	
		if(recive(ds_socket,&check,sizeof(check),0)==-1) {//attendo la risposta del server alla nostra richiesta di autenticazione
			printf("Errore nella ricezione dell'esito dell'autenticazione\n");
			raise(SIGILL);
			}
		
		
		if(check=='0'){
			printf("Nessun riscontro, registrato come nuovo utente\n");
			}
		if(check=='1'){
			printf("L'autenticazione ha dato buon esito, bentornato\n");
			}
		if(check=='2'){
			printf("ERRORE: Allo user inviato è associata un altra password\n");
			raise(SIGILL);
			}
		//Fase preliminare conclusa
	
		//Implementazione delle operazioni implementate dalla bacheca elettronica
	

		switch(req.op){
			case '1': //lettura dei messaggi della bacheca
				counter=0;
				do {
					
					memset(buffer,0,sizeof(buffer)); //azzero il buffer dal precedente contenuto informativo
					if(result=recive(ds_socket, buffer,sizeof(buffer))==-1){
						printf("Errore nella ricezione dei messaggi dal server\n");
						raise(SIGILL);
					}
					if(buffer[0]=='#'&&buffer[1]=='\0')
						result=0;
					else
						printf("%d: %s",counter,buffer);
					counter++;
					}while(result>0);
				close(ds_socket);
				break;
					
			case '2': //Invio nuovo messaggio nella bacheca
				recive(ds_socket,&check,sizeof(check),0);
				if(check=='0'){
					printf("Impossibile aggiungere il post\n");
					}
				if(check=='1')
					printf("Aggiunta dalla bacheca effettuata\n");
				close(ds_socket);
				break;
			case '3': //rimozione messaggio da bacheca
				recive(ds_socket,&check,sizeof(check),0);
				if(check=='0'){
					printf("Impossibile cancellare il post (post non presente o non è un tuo post)\n");
					}
				if(check=='1')
					printf("Rimozione dalla bacheca effettuata\n");
				close(ds_socket);
				break;
			case '4': //chiusura della applicazione
				printf("Chiusura richiesta dell'applicazione da parte dell'utente\n"); //in realtà non può accadere perché si sarebbe usciti dal programma precedentemente
				raise(SIGILL);
			}
		}while(1==1);
	}
		
	//Metodi Ausiliari 
	//legge una stringa e la tronca alla massima lunghezza accettabile, scartando il resto
	int read_and_check(char*buff_aux,int len){
		memset(buff_aux,0,sizeof(buff_aux));
		if(fgets(buff_aux,len,stdin)==-1){
			printf("Errore occorso in fase di lettura\n");
			return -1;
			}
		if(buff_aux[0]=='#')
			return -1;
		if(buff_aux[strlen(buff_aux-1)]=='\n') 
			buff_aux[strlen(buff_aux-1)]='\0';
		else
			while(getchar()!='\n');
		return strlen(buff_aux);
		}
	
	int delive(int ds, char buff[],int len){
		int wr,left;
		char *buffer;
		buffer=buff;
		left=len;
		while(left>0){
			if((wr=write(ds,buffer,left))==-1){
				printf("Delive: Errore in fase di scrittura\n");
				return -1;
				}
			left=left-wr;
			buffer=buffer+wr;
			}
		return len;
		}
	
	int recive(int ds, char buff[], int len) {
		int rd, left;
		char *buffer;
		buffer=buff;
		left=len;
		alarm(TIMEOUT);
		if(rd=read(ds,buffer,left)==-1){
			printf("Errore in fase di lettura\n");
			return -1;
			}
		alarm(0);
		return len;
		}
