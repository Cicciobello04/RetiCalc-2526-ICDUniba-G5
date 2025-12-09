#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined _WIN32
    #include <winsock.h> // 
#else
    #define closesocket close
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h> // [cite: 71]
#endif

#define BUFFERSIZE 512

void ClearWinSock() {
#if defined _WIN32
    WSACleanup();
#endif
}

int main(void) {
#if defined _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData); // [cite: 76]
    if (iResult != 0) {
        printf("Error at WSAStartup()\n");
        return EXIT_FAILURE;
    }
#endif

    int Csocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (Csocket < 0) {
        printf("Error at socket()\n");
        ClearWinSock();
        return EXIT_FAILURE;
    }

    // 1. Lettura nome host e porta
    char serverNameStr[100];
    int port;
    
    printf("Inserisci il NOME dell'host del server (es. localhost): ");
    scanf("%99s", serverNameStr);
    
    printf("Inserisci la porta del server (es. 27015): ");
    scanf("%d", &port);
    getchar(); // Consuma newline

    // --- USO DI gethostbyname (PDF pag. 3 e 8) ---
    // Risoluzione da nome simbolico a indirizzo Internet [cite: 14]
    struct hostent *host;
    host = gethostbyname(serverNameStr); // [cite: 17, 85]

    if (host == NULL) { // [cite: 19, 86]
        fprintf(stderr, "gethostbyname() failed. Host non trovato.\n");
        closesocket(Csocket);
        ClearWinSock();
        return EXIT_FAILURE;
    }

    struct sockaddr_in sad;
    memset(&sad, 0, sizeof(sad));
    sad.sin_family = AF_INET;
    sad.sin_port = htons(port);
    // Copia dell'indirizzo dalla struttura hostent alla struct sockaddr_in
    // h_addr è definito come h_addr_list[0] [cite: 30]
    memcpy(&sad.sin_addr, host->h_addr, host->h_length); 

    int serverLen = sizeof(sad);

    // 2. Invio "Hello"
    const char *handshake = "Hello";
    sendto(Csocket, handshake, strlen(handshake), 0, (struct sockaddr*)&sad, serverLen);
    printf("Invio messaggio: %s\n", handshake);

    // 4. Lettura stringa da tastiera
    char inputString[BUFFERSIZE];
    printf("Inserisci una stringa da elaborare: ");
    if (fgets(inputString, BUFFERSIZE, stdin) != NULL) {
        inputString[strcspn(inputString, "\n")] = 0;
        
        sendto(Csocket, inputString, strlen(inputString), 0, (struct sockaddr*)&sad, serverLen);
        printf("Stringa inviata. In attesa...\n");
    }

    // 7. Ricezione risposta e visualizzazione info Server
    char response[BUFFERSIZE];
    struct sockaddr_in fromAddr;
    #if defined _WIN32
        int fromLen = sizeof(fromAddr);
    #else
        socklen_t fromLen = sizeof(fromAddr);
    #endif

    int bytesRcvd = recvfrom(Csocket, response, BUFFERSIZE - 1, 0, (struct sockaddr*)&fromAddr, &fromLen);

    if (bytesRcvd > 0) {
        response[bytesRcvd] = '\0';
        
        // Risoluzione INVERSA: dall'indirizzo IP ricevuto al nome del server
        // Uso gethostbyaddr come da PDF pag. 5 
        struct hostent *serverInfo;
        serverInfo = gethostbyaddr((char *)&fromAddr.sin_addr, 4, AF_INET);

        char *srvName = (serverInfo != NULL) ? serverInfo->h_name : "Sconosciuto";

        printf("\nRISPOSTA DAL SERVER:\n");
        printf("Stringa '%s' ricevuta dal server\n", response);
        printf("Nome: %s\n", srvName);
        printf("Indirizzo: %s\n", inet_ntoa(fromAddr.sin_addr)); // [cite: 93]
    }

    closesocket(Csocket);
    ClearWinSock();
    return 0;
}