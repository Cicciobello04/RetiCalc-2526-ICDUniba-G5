#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined _WIN32
    #include <winsock.h> //
#else
    #define closesocket close
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h> //
#endif

#define PROTOPORT 27015
#define BUFFERSIZE 512

void ClearWinSock() {
#if defined _WIN32
    WSACleanup();
#endif
}

// Funzione helper per le vocali
int vocale(char c) {
    c = tolower((unsigned char)c);
    return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u');
}

void rimuovi_vocali(char *str) {
    if (str == NULL) return;
    int l = 0, s = 0;
    while (str[l] != '\0') {
        if (!vocale(str[l])) {
            str[s] = str[l];
            s++;
        }
        l++;
    }
    str[s] = '\0';
}

int main(int argc, char **argv) {
    int port = PROTOPORT;
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    // --- Inizializzazione Winsock (come da slide 7) ---
#if defined _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("Error at WSAStartup()\n");
        return EXIT_FAILURE;
    }
#endif

    int MySocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    struct sockaddr_in sad;
    memset(&sad, 0, sizeof(sad));
    sad.sin_family = AF_INET;
    sad.sin_addr.s_addr = htonl(INADDR_ANY);
    sad.sin_port = htons(port);

    if (bind(MySocket, (struct sockaddr*) &sad, sizeof(sad)) < 0) {
        printf("bind() failed\n");
        closesocket(MySocket);
        ClearWinSock();
        return EXIT_FAILURE;
    }

    struct sockaddr_in cad;
    int clientLen = sizeof(cad);
    char buf[BUFFERSIZE];

    printf("Server UDP in ascolto sulla porta %d...\n", port);

    while (1) {
        memset(buf, 0, BUFFERSIZE);
        
        #if defined _WIN32
            int len = sizeof(cad);
        #else
            socklen_t len = sizeof(cad);
        #endif

        int bytesRcvd = recvfrom(MySocket, buf, BUFFERSIZE - 1, 0, (struct sockaddr*)&cad, &len);
        
        if (bytesRcvd > 0) {
            buf[bytesRcvd] = '\0';

            // --- USO DELLE FUNZIONI DEL PDF PER RISOLVERE IL NOME ---
            // Usiamo gethostbyaddr per passare da IP a Nome Simbolico
            // Signature: struct hostent * gethostbyaddr(const char* addr, int len, int type);
            
            struct hostent *remoteHost;
            remoteHost = gethostbyaddr((char *)&cad.sin_addr, 4, AF_INET);

            char *clientName;
            if (remoteHost != NULL) {
                clientName = remoteHost->h_name; //
            } else {
                clientName = "Sconosciuto (risoluzione fallita)";
            }

            // Visualizzazione dati client (Punto 3 della traccia)
            printf("\n------------------------------------------------\n");
            printf("Ricevuti dati dal client.\n");
            printf("Messaggio: '%s'\n", buf);
            printf("Nome Host: %s\n", clientName);
            // inet_ntoa converte l'indirizzo numerico in stringa
            printf("Indirizzo IP: %s\n", inet_ntoa(cad.sin_addr)); 

            if (strcmp(buf, "Hello") != 0) {
                // Elaborazione (rimozione vocali)
                rimuovi_vocali(buf);
                
                sendto(MySocket, buf, strlen(buf), 0, (struct sockaddr*)&cad, len);
                printf("Risposta inviata: '%s'\n", buf);
            }
            printf("------------------------------------------------\n");
        }
    }

    closesocket(MySocket);
    ClearWinSock();
    return 0;
}