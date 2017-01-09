#Tesina:
Bacheca Elettronica Remota

#Sistema Operativo:
Unix

Realizzazione di una bacheca elettronica residente su un server. Una
bacheca elettronica e' un servizio che permette ad ogni utente autorizzato
(su qualunque macchina) di inviare messaggi che possono essere letti da
ogni altro utente interessato a consultare la bacheca stessa. In questo
caso la bacheca e' costituita da un programma server che accetta e
processa sequenzialmente le richieste di uno o piu' processi client
(residenti, in generale, su macchine diverse dal server). Un client deve
fornire ad un utente le seguenti funzioni:

1- Leggere tutti i messaggi sulla bacheca elettronica.
2- Spedire un nuovo messaggio sulla bacheca elettronica.
3- Rimuovere un messaggio dalla bacheca elettronica (se appartenente
allo stesso utente che lo vuole cancellare, verifica da effettuare tramite un meccanismo di autenticazione a
scelta dello studente).
Un messaggio deve contenere almeno i campi Mittente, Oggetto e Testo. Si
precisa che lo studente e' tenuto a realizzare sia il client che il
server.



# Cenni preliminari:
Il progetto è stato strutturato nella forma di client-server singolo processo.
La comunicazione tra i due è affidata ad una connessione TCP (connessione affidabile).
Scelte Implementative
Per rendere persistenti le informazioni nel server, si utilizza la scrittura su due file. Account tiene conto degli utenti noti e delle loro password, mentre Bacheca rappresenta la sequenza di post presenti nella Bacheca Remota.
Come unità di scambio tra il client e server si è scelto di utilizzare una “richiesta” ovvero un tipo strutturato contenente sia le operazioni da eseguire, sia le credenziali dell’utente che le richiede. E’ altresi presente una struttura ad unico utilizzo del client “user_data” che mantiene durante la vita stessa del client traccia delle credenziali dell’utente.
Questi due tipi di dato strutturato, sono stati definiti come segue:
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

# Comportamento Generale
Il server una volta avviato rimane in attesa di connessioni in arrivo.
Quando un client viene avviato, al utente viene richiesto di autenticarsi per poter accedere alle funzionalità della bacheca remota.
Viene in seguito richiesto di scegliere da un elenco di operazioni, quella desiderata e a seguito di quella scelta, l’immissione di ulteriori dati per finalizzarla (se necessario).
A questo punto i dati inseriti hanno compilato correttamente una “richiesta” e il client si collega al server per inviarla.
Il server estrae dalla richiesta le credenziali del utente e le confronta con quelle presenti in “Account”, e può discriminare tra: Utente Riconosciuto, Password Errata (condizione di errore) e Nuovo Utente.
Notifica al client l’esito del autenticazione tramite un codice numerico e se non si è verificata una condizione di errore procede all’erogazione del servizio.
Se si è verificata una condizione di errore, la richiesta non verrà ulteriormente processata. La connessione con il client verrà terminata e si tornerà in attesa di una nuova connessione. Una ricezione di Password Errata al Client lo fa terminare.
Una volta che il server ha erogato il servizio richiesto chiude la connessione e si rimette in attesa di un client da servire.
Il client allo stesso modo una volta ricevuto la risposta del server, si rimette in attesa della prossima operazione da effettuare.

# Operazioni erogate: Lato Client
All’utente dopo aver inserito le proprie credenziali viene richiesto di specificare l’operazione da eseguire, le operazioni che il client può scegliere sono:
1- Visionare i post della Bacheca: Senza la richiesta di ulteriori dati, la richiesta viene inviata al Server che restituirà una sequenza di post numerati presenti nella bacheca elettronica (terminata da una recive di un buffer che inizia con “#\0”) e li mostrerà a schermo.
2- Aggiungere nuovo post: Vengono richiesti, oggetto e testo del post per poi inviare la richiesta completa al server, viene notificato al client il successo/fallimento dell’operazione di aggiunta.
3- Rimozione Post: Viene richiesto il numero del post e la richiesta viene inviata al server, viene notificato il fallimento dell’operazione di rimozione.
4- Chiusura del client: Viene semplicemente avviata una routine di Showtdown.

# Operazioni Erogate: Lato Server
Il server a ogni connessione con un client, riceve una richiesta. Se non si verifica una condizione di errore nell’autenticazione la soddisfa in base a quale sia l’operazione richiesta.
1-	Visionare i post della Bacheca: Il server apre il file Bacheca con i permessi di lettura e inizia a leggerlo post per post, se il post non inizia con il carattere speciale di un post cancellato. Lo invia attraverso il socket al client. Questo finché il file non è terminato (non ci sono più post). A questo punto il file viene chiuso e viene inviati ulteriori dati, contenenti il codice speciale. Poi la connessione con il client viene chiusa.
2-	Aggiungere nuovo post: Il server apre il file Bacheca con i permessi di lettura/scrittura e inizia a leggerlo post per post in cerca di uno spazio libero dove inserire il nuovo post. Per “spazio libero” si intende uno spazio identificato dal carattere speciale (segno che un post che si trovava lì è stato virtualmente cancellato) o la fine del file. Trovato lo “spazio” il server scrive lì il nuovo post. Chiude il file, invia una notifica al client sul esito e chiude la connessione con il client.
3-	Rimozione post: Il server apre il file Bacheca con i permessi di lettura/scrittura e inizia a leggerlo n volte (dove n è pari al numero del post da eliminare). Se l’n-esimo post esiste ed è stato inviato dal utente attuale viene rimosso (scrivendo al suo inizio il carattere speciale). Dopodiché il file viene chiuso, una notifica sul esito inviata al client e la connessione con il client chiusa.

# Gestioni Timeout:
Data l’asimmetria nei ruoli tra client e server. Ho optato per comportamenti molto diversi nel caso in cui una delle due parti non riceva più responsi dall’altra.
Client: la gestione_alarm() quando invocata, si limita a notificare a schermo che il timeout è scaduto, chiude il socket e termina il client.
Server: la gestione_connessione_inattiva() quando invocata, notifica a schermo che il timeout è scaduto, chiude il socket con il client (ma non il listener socket) e setta una variabile che farà tornare il server disponibile per nuove connessioni (nel corso di ciò la variabile verrà resettata). 
