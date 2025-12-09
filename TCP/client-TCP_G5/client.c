#include <stdio.h>
#include <stdlib.h> // for atoi()
#include <string.h> // per strlen, strcmp, strcspn

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
#endif

#define PROTOPORT 5193 // default protocol port number (non usato qui)
#define BUFFERSIZE 512 // size of request queue
#define SERVERPORT 27015

// Funzione di utilità per pulire Winsock, chiamata solo se in ambiente Windows
void ClearWinSock() {
#ifdef _WIN32
    WSACleanup();
#endif
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
        port = SERVERPORT;
        if (port < 0) {
            // Questo blocco non sarà mai eseguito se port = SERVERPORT (27015)
            printf("bad port number %s \n", argv[1]);
            return 1;
        }
    }

    // --- Inizializzazione Winsock (Solo Windows) ---
    #ifdef _WIN32
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2 ,2), &wsaData);
        if (iResult != 0) {
            printf("error at WSAStartup\n");
            return -1;
        }
    #endif
    // ----------------------------------------------

    int Csocket;
    Csocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (Csocket < 1) { 
        printf("Error at socket()");
        closesocket(Csocket);
        ClearWinSock();
        return -1;
    }

    // COSTRUZIONE DELL’INDIRIZZO DEL SERVER
    struct sockaddr_in sad;
    memset(&sad, 0, sizeof(sad));
    sad.sin_family = AF_INET;
    sad.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP del server
    sad.sin_port = htons(port); // Server port

    // CONNESSIONE AL SERVER
    if (connect(Csocket, 
        (struct sockaddr *)&sad, sizeof(sad)) < 0) {
            printf("Error at connect(). Controlla che il server sia in ascolto");
            closesocket(Csocket);
            ClearWinSock();
            return -1;
    }

    // 2) Stabilita la connessione, il server invia al client la stringa
        char response[BUFFERSIZE];
        sprintf(response, "Hello!");
        send(Csocket, response, strlen(response), 0);


    // INSERIMENTO MESSAGGIO DA TASTIERA
    printf("\n--- Ciclo di comunicazione --- \n");
    printf("Inserisci una stringa da inviare al server:\n");
    char inputString1[BUFFERSIZE];
    
    while(1) {
        
        // 3) Legge una stringa dallo standard input

        // INSERIMENTO
        printf("> ");
        if (fgets(inputString1, BUFFERSIZE, stdin) == NULL) {
            break; // Interruzione input
        }
        
        
        // Rimuove il newline finale (\n) se presente, usando strcspn come best practice
        inputString1[strcspn(inputString1, "\n")] = 0;
        
        // INVIO STRINGA AL SERVER
        int stringLen1 = strlen(inputString1);
        if (send(Csocket, inputString1, stringLen1, 0) != stringLen1) {
            printf("Errore nell'invio della prima stringa");
            break;
        }        
        printf("-> Stringa inviata. In attesa di risposta dal server...\n\n");

        char buf1_rcv[BUFFERSIZE];
        int bytesRcvd;

        
        // RICEVE LA RISPOSTA
        bytesRcvd = recv(Csocket, buf1_rcv, BUFFERSIZE - 1, 0);
        if (bytesRcvd > 0) {
            buf1_rcv[bytesRcvd] = '\0';
            printf("RISPOSTA >> '%s'\n", buf1_rcv);
            closesocket(Csocket);
            ClearWinSock();
            printf("--- Fine scambio ---\n\n");
            break;
        } else if (bytesRcvd == 0) {
            printf("Server ha chiuso la connessione inaspettatamente.\n");
            break;
        } else {
             perror("Errore nella ricezione della prima risposta");
             break;
        }
    }

    closesocket(Csocket);
    ClearWinSock();
    printf("\n");

    return 0;
}