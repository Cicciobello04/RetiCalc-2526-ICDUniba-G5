#include <stdio.h>
#include <stdlib.h> // for atoi()
#include <string.h>
#include <ctype.h> 

#ifdef _WIN32
// Inclusione per Windows
#include <winsock.h> 
#else
// Equivalenti POSIX (Linux/macOS)
#define closesocket close // Definisci closesocket come close
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // for close()
#include <errno.h> // *** NUOVA INCLUSIONE POSIX: Importante per la gestione errori ***
#endif

#define PROTOPORT 27015
#define QLEN 6
#define BUFFERSIZE 512

// Funzione di utilità per pulire Winsock, chiamata solo se in ambiente Windows
void ClearWinSock() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// Funzione helper per verificare se un char è una vocale
int vocale(char c) {
    // Convertiamo in minuscolo per fare meno controlli
    c = tolower((unsigned char)c);
    // Verifichiamo se c è presente nella stringa delle vocali
    return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u');
}


void rimuovi_vocali(char *str) {
    if (str == NULL) return;

    int l = 0; // Indice di lettura
    int s = 0; // Indice di scrittura

    while (str[l] != '\0') {
        // Se il carattere NON è una vocale, lo scriviamo
        // nella posizione di scrittura e avanziamo s
        if (!vocale(str[l])) {
            str[s] = str[l];
            s++;
        }
        // L'indice di lettura avanza sempre
        l++;
    }
    
    // IMPORTANTE: Terminiamo la nuova stringa con il terminatore nullo
    str[s] = '\0';
}

int main(int argc, char **argv) {

    int port;
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port < 0) {
            printf("bad port number %s \n", argv[1]);
            return 1;
        }
    } else {
        port = PROTOPORT;
        if (port < 0) {
            printf("bad port number %s \n", argv[1]);
            return 1;
        }
    }

    // --- Inizializzazione Winsock (Solo Windows) ---
#ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2 ,2), &wsaData);
    if (iResult != 0) {
        printf("Error at WSAStartup()\n");
        return -1;
    }
#endif
    // ----------------------------------------------

    int MySocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (MySocket < 0) {
        printf("Error at socket() of variable MySocket\n");
        ClearWinSock();
        return -1;
    }

    // ASSEGNAZIONE DI UN INDIRIZZO ALLA SOCKET
    struct sockaddr_in sad;
    memset(&sad, 0, sizeof(sad));
    sad.sin_family = AF_INET;
    // sad.sin_addr.s_addr = inet_addr("127.0.0.1"); // Originale
    sad.sin_addr.s_addr = htonl(INADDR_ANY); // *** MODIFICA: In ascolto su tutte le interfacce ***
    sad.sin_port = htons(port);

    if (bind(MySocket, (struct sockaddr*) &sad, sizeof(sad)) < 0 ) {
        printf("bind() failed\n");
        closesocket(MySocket);
        ClearWinSock();
        return -1;
    }

    // SETTAGGIO DELLA SOCKET ALL'ASCOLTO
    if (listen(MySocket, QLEN) < 0) {
        printf("listen() failed\n");
        closesocket(MySocket);
        ClearWinSock();
        return -1;
    }

    // ACCETTARE UNA NUOVA CONNESSIONE
    struct sockaddr_in cad;
    int clientSocket;
    
    #ifdef _WIN32
        int clientLen;
    #else
        socklen_t clientLen;
    #endif

    char buf[BUFFERSIZE]; 
    int bytesRcvd;

    printf("Server in ascolto sulla porta %d...\n", port);
    
    while(1) {
        clientLen = sizeof(cad);
        if ((clientSocket = accept(MySocket, 
            (struct sockaddr *)&cad, &clientLen)) < 0) {
                perror("accept() failed");
                closesocket(MySocket);
                ClearWinSock();
                return 1;
            }

        // 2) Riceve la stringa "Hello" dal client
        char response[BUFFERSIZE];
        bytesRcvd = recv(clientSocket, response, BUFFERSIZE - 1, 0);
        if (bytesRcvd > 0) {
            response[bytesRcvd] = '\0';
            // 3) Stampa la stringa ricevuta
            printf("Client: '%s'\n", response);
            printf("Ricevuti dati dal client con indirizzo: %s\n", inet_ntoa(cad.sin_addr));
        } else if (bytesRcvd == 0) {
            printf("Server ha chiuso la connessione prima dell'inizio delle operazioni\n");
            closesocket(clientSocket);
            ClearWinSock();
            return 0;
        } else {
            printf("Errore nella ricezione iniziale");
            closesocket(clientSocket);
            ClearWinSock();
            return -1;
        }

        // LOOP PER GESTIRE MULTIPLI MESSAGGI DAL CLIENT
        while(1) {
            
            // *** MODIFICA: Ciclo robusto per la RICEZIONE della STRINGA ***
            do {
                bytesRcvd = recv(clientSocket, buf, BUFFERSIZE - 1, 0);
                #ifndef _WIN32
                if (bytesRcvd < 0 && errno == EINTR) { 
                    continue; // Interrotto da un segnale, riprova
                }
                #endif
                if (bytesRcvd < 0) {
                     perror("Errore nella ricezione della prima stringa");
                }
            } while (bytesRcvd < 0 && 
            #ifndef _WIN32
                    errno == EINTR
            #else
                    0 // Su Windows non gestiamo EINTR esplicitamente
            #endif
            );

            if (bytesRcvd <= 0) break; // Connessione chiusa o errore grave
            buf[bytesRcvd] = '\0';

            // 4) Il server legge le stringhe inviate dal client e le visualizza
            printf("\n--- Messaggio Ricevuto --- \n");
            printf(">> '%s'\n", buf);
            printf("------------------------ \n\n");

            rimuovi_vocali(buf);
                        
            printf("--- Messaggio Convertito --- \n");
            printf(">> '%s'\n", buf);
            printf("------------------------ \n\n");

            // INVIO PRIMA STRINGA
            if (send(clientSocket, buf, strlen(buf), 0) != (int)strlen(buf)) {
                 perror("Errore nell'invio della stringa modificata");
                 break;
            }            
            printf("Risposta inviata al client.\n");
            printf("------------------------ \n\n");
            
        }
        
        closesocket(clientSocket);
        printf("Connessione con %s chiusa\n\n", inet_ntoa(cad.sin_addr));
        /* break; togli il commento se vuoi che il server venga chiuso
                  quando viene chiuso il client */
    }

    closesocket(MySocket);
    ClearWinSock();

    return 0;
}