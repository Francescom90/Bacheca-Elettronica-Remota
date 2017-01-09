#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/signal.h>


#define USER 16
#define PW 16
#define OGGETTO 32
#define TESTO 900
#define BACKLOG 10
#define TIMEOUT 2

//Definizione dei tipi strutturati necessari
typedef struct {
	char op;
	char user_name[USER];
	char user_pw[PW];
	char target_post;
	char mittente[USER];
	char oggetto[OGGETTO];
	char testo[TESTO];
	} richiesta;

int ds_listener;
int ds_sock;

//Funzioni per la gestione dei segnali
void gestione_chiusura(){
	printf("E' occorso un problema, il server verrà terminato\n");
	close(ds_sock);
	close(ds_listerner);
	exit(0);
	}
void gestione_chiusura_server(){
	printf("Il server sta eseguendo lo shoutdown\n");
	close(ds_sock);
	close(ds_listener);
	exit(0);
	}

int salarm=0;
void gestione_connessione_inattiva(){
	printf("Il client non risponde, la connessione con lui sarà interrotta\n");
	close(ds_sock);
	salarm=1;	
	}

//prototipo per la funzione autenticazione
char autenticazione(char*,char*);

// Funzione main

void main(int argc, char*argv[]) {
	printf("Startup del server\n");
	//dichiarazione variabili locali
	char check;
	int result;
	richiesta req;
	char buffer[TESTO];
	char buffer_aut[USER];
	short int port;
	int f_cred;
	int f_bacheca;
	int counter;
	struct sockaddr_in client;
	sigset_t sset;

	//gestione segnali
	signal(SIGHUP, gestione_chiusura);
	signal(SIGINT, gestione_chiusura);
	signal(SIGQUIT, gestione_chiusura);
	signal(SIGILL, gestione_chiusura);
	signal(SIGSEGV, gestione_chiusura);
	signal(SIGTERM, gestione_chiusura_server);
	signal(SIGALRM, gestione_connessione_inattiva);
	signal(SIGCHLD, SIG_IGN);
	
	memset(&sset, 0, sizeof(sigset_t));

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
	
	//apertura del file delle cerdenziali

	f_bacheca=open("bacheca",O_CREAT|O_RDONLY,0600); //serve in realtà ad assicurarsi che il file esista e in caso non esiste lo crea in maniera contingente
	if(f_bacheca==-1){
		printf("Impossibile accedere al file della bacheca\n");
		raise(SIGILL);
		}
	close(f_bacheca);
	
	//Creazione socket
	struct sockaddr_in server;
	ds_listener=socket(AF_INET,SOCK_STREAM,0);
	if(ds_listener==-1){
		printf("Impossibile creare il socket\n");
		exit(0);
		}
	printf("Socket listener creato\n");
	//Indirizzzo server
	memset((void*)&server,0,sizeof(server));
	server.sin_family=AF_INET;
	server.sin_port=htons(7777);
	server.sin_addr.s_addr=INADDR_ANY;
	
	//binding del socket
	if(bind(ds_listener,(struct socketaddr*)&server,sizeof(server))==-1) {
		printf("Errore durante il Binding \n");
		raise(SIGILL);
	}
	printf("Binding del listener Socket effettuato");
	//server messo in ascolto
	if(listen(ds_listener,BACKLOG)==-1){
		printf("Errore durante il Listening\n");
		raise(SIGILL);
		}
	printf("Il server è stato correttamente inizializzato e attende connessioni\n");
	
	
	while(1==1){
		salarm=0;
		memset(&client,0,sizeof(client));
		printf("Il server è pronto a ricevere nuove connessioni\n");
		//attesa di connessioni, accept
		ds_sock=accept(ds_listener, (struct sockaddr *)&client,sizeof(client));
		if(ds_sock==-1){
			printf("Impossibile stabilire una connessione con un client\n");
			raise(SIGILL);
			}
		printf("Accettata connessione con un client\n");
		//ricezione della richiesta del client
		memset((void*)&req,0,sizeof(req));
		recive(ds_sock,&req,sizeof(req));
		check=autenticazione(req.user_name,req.user_pw);
		
		if(delive(ds_sock,&check,sizeof(check))==-1){ //invio la risoluzione dell'autenticazione e controllo la riuscita del invio
				printf("Impossibile inviare l'esito\n");
				raise(SIGILL);
				}
		if(check=='2'){
			printf("Credenziali del client errate, termino la connessione\n");
			close(ds_sock);
			}
			
		if(check!='2'&& salarm==0){ //se il client è valido e non si sono verificati timeout durante la recive
				
			switch(req.op) {
			case '1': //Operazione richiesta:Lettura Bacheca
				printf("Il client ha richiesto la visualizzazione della bacheca\n");
				if(f_bacheca=open("bacheca",O_RDONLY)==-1){ //apro il file contentente i messaggi con i permessi di lettura
					printf("Errore nel accesso al file Bacheca\n");
					raise(SIGILL);
					}
				do {
					memset(buffer,0,sizeof(buffer));
					if(result=read(f_bacheca,buffer,sizeof(post_data)+3)==-1){ //leggo una porzione di file contenente un post
						printf("Errore nella lettura della bacheca da file\n");
						raise(SIGILL);
						}
					if(result>0&&buffer[0]!='#'&& buffer[1]!='\0'){ //le il file non è terminato e non ho letto i tag di "post cancellato"
					
						if(delive(ds_sock,&buffer,result)==-1){ //invio i dati al client
							printf("Errore nel invio dei dati al client\n");
							raise(SIGILL);
							}
						}

					}while(result>0);
				if(result==0){   //se il file è finito, scrivo nel buffer un codice particolare che indica la fine dei post
					buffer[0]='#';
					buffer[1]='\0';
					if(delive(ds_sock,&buffer,sizeof(buffer))==-1){
						print("Errore nei invio dei dati al client\n");
						raise(SIGILL);
						}
					}
				if(close(f_bacheca)==-1){
					printf("Errore nella chiusura del file Bacheca\n");
					raise(SIGILL);
					}
				close(ds_sock);
				printf("La richiesta del client è stata servita, chiudo la connessione\n");
				break;
			case '2': //Operazione richiesta: Invio nuovo messaggio alla bacheca
				printf("Il client ha richiesto l'invio di un nuovo post nella bacheca\n");
				if(f_bacheca=open("bacheca",O_RDWR)==-1){  //apro il file contenente i messaggi con i permessi di scrittura e lettura
					printf("Errore nel accesso al file Bacheca\n");
					raise(SIGILL);
					}
				do {
					memset(buffer,0,sizeof(buffer));
					if(result=read(f_bacheca,buffer,sizeof(post_data)+3)==-1){ //leggo una porzione di file contenente un post
						printf("Errore nella lettura della bacheca da file\n");
						raise(SIGILL);
						}
					}while(result>0 && buffer[0]!='#' && buffer[1]!='\0'); //scorro il file finche non trovo un post precendentemente cancellato o la fine del file
				write(f_bacheca,strcat(req.pd.mittente,"\n"),USER+1); //aggiungo nella locazione il nuovo post
				write(f_bacheca,strcat(req.pd.oggetto,"\n"),OGGETTO+1);
				write(f_bacheca,strcat(req.pd.testo,"\n"),TESTO+1);
					
				if(close(f_bacheca)==-1){
					printf("Errore nella chiusura del file Bacheca\n");						
					raise(SIGILL);
					}
				check='1';		
				delive(ds_sock,&check,sizeof(check));
				close(ds_sock);
				printf("La richiesta del client è stata servita, chiudo la connessione\n");
				break;
				
			case '3': //Operazione richiesta: Rimozione messaggio da bacheca
				printf("Il client ha richiesto l'eliminazione di un suo messaggio dalla bacheca\n");
				if(f_bacheca=open("bacheca",O_RDWR)==-1){  //apro il file contenente i messaggi con i permessi di scrittura e lettura
					printf("Errore nel accesso al file Bacheca\n");
					raise(SIGILL);
					}
				for(counter=0; counter<req.target_post;){
					memset(buffer,0,sizeof(buffer));
					if(result=read(f_bacheca,buffer,sizeof(post_data)+3)==-1){ //leggo una porzione di file contenente un post
						printf("Errore nella lettura della bacheca da file\n");
						raise(SIGILL);
						}
					if(buffer[0]!='#'&&buffer[1]!='\n')
						counter++;
					}
					
				read(f_bacheca,buffer_aut,USER);
				if(buffer_aut!=req.ud.user){
					check='0';
					if(close(f_bacheca)==-1){
						printf("Errore nella chiusura del file Bacheca\n");						
						raise(SIGILL);
						}
					if(f_bacheca=open("bacheca",O_RDWR)==-1){ 
						printf("Errore nel accesso al file Bacheca\n");
						raise(SIGILL);
						}
					for(counter=0; counter<req.target_post-1;){
						memset(buffer,0,sizeof(buffer));
						if(result=read(f_bacheca,buffer,sizeof(post_data)+3)==-1){ 
							printf("Errore nella lettura della bacheca da file\n");
							raise(SIGILL);
							}
						if(buffer[0]!='#'&&buffer[1]!='\n')
							counter++;
						}
					buffer[0]='#';
					buffer[1]='\n';
					write(f_bacheca,buffer,2);
					}

				else
					check='1';
						
				delive(ds_sock,&check,sizeof(check));
					
				if(close(f_bacheca)==-1){
					printf("Errore nella chiusura del file Bacheca\n");						
					raise(SIGILL);
					}
				close(ds_sock);
				printf("La richiesta del client è stata servita, chiudo la connessione\n");
				break;
				}
			}
		}
	}
				
	//Metodi Ausiliari 
	//legge una stringa e la tronca alla massima lunghezza accettabile, scartando il resto
	int read_and_check(char*buff_aux,int len){
		memset(buff_aux,0,sizeof(buff_aux));
		if(fgets(buff_aux,len,stdin)==-1){
			printf("Errore occorso in fase di lettura\n");
			return -1;
			}
		if(buff_aux[strlen(buff_aux-1)]=='\n') 
			buff_aux[strlen(buff_aux-1)]='\0';
		else
			while(getchar()=='\n');
		return strlen(buff_aux);
		}
	
	int delive(int ds, char buff[],int len){
		int wr,left;
		char *buffer;
		buffer=buff;
		left=len;
		while(left>0){
			if((wr=write(ds,buffer,left))==-1){
				printf("Errore in fase di scrittura\n");
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
		alarm(TIMEOUT); //visto che nel client i dati vengono spediti subito dopo la connect (essendo già pronti), questa Alarm non dovrebbe mai generare un SIGALARM, ma è meglio gestire comunque la cosa
		if(rd=read(ds,buffer,left)==-1){
			printf("Errore in fase di lettura\n");
			return -1;
			}
		alarm(0);
		return len;
		}
	
	char autenticazione(char* nome, char* pass, int f_cred) {
		char* credenziali_n=NULL;
		char* credenziali_p=NULL;
		size_t len = 0;
		f_cred=open("account", O_APPEND|O_RDWR|O_CREAT, 0600);
		if(f_cred==-1){
			printf("Impossibile accedere al file delle credenziali\n");
			raise(SIGILL);
			}
		while(readline(&credenziali_n,&len,f_cred)!=-1){ 
			readline(&credenziali_p,&len,f_cred);
			if(strcmp(credenziali_n,nome)){      //se ho riscontro con gli user
				if(strcmp(credenziali_p,pass)){	 //controllo se le password corrispondono
					return '1';					 //credenziali riscontrate
					}
				else {
					return '2';					 //credenziali sbagliate
				}
			}
		free(credenziali_n);
		free(credenziali_p);
		write(f_cred,strcat(nome,"\n"),17); //aggiungo la nuova utenza
		write(f_cred,strcat(pass,"\n"),17);
		close(f_cred);
		return '0';
		}
