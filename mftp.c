#include "mftp.h"

static const BUFFER_SIZE = 1024;
/**** !!!!!!!!!!!!!!!!!! KAN SPARE MANGE LINJER 
***************** sprintf(str, "%d", aInt);
**/
void ftpClient() {
    int control_socket = 0, i;
    char recvBuff[BUFFER_SIZE], sendBuff[BUFFER_SIZE];

    memset(recvBuff, '0', sizeof(recvBuff));
    memset(sendBuff, '0', sizeof(sendBuff));

    control_socket = connectSocket(gArgs.port);
    
    authentificate(control_socket, recvBuff, sendBuff);

    strcpy(sendBuff, "SYST\r\n");
    logWrite(control_socket, sendBuff);

    logRead(control_socket, recvBuff);

    setType(control_socket, sendBuff);

    logRead(control_socket, recvBuff);

    retriveFile(sendBuff, recvBuff, control_socket);
    
    logRead(control_socket, recvBuff);

//   if (strncmp(recvBuff, "150  ", 3) == 0)

}

int findPasvPort(char searchString[]) {
    char b[100];
    char e[100];
    char tmp[10];
    int offset;
    int secondlast, last, port;

    memset(b, '0',sizeof(b));
    memset(e, '0',sizeof(e));

    substrafter(b, searchString, ",", 4);
    substrafter(e, searchString, ",", 5);

    offset = strlen(b)-strlen(e)-1;

    b[offset] = 0;
    secondlast = atoi(b);

    substrafter(b, searchString, ",", 5);
    substrafter(e, searchString, ")", 1);

    offset = strlen(b)-strlen(e)-1;

    b[offset] = 0;
    last = atoi(b);

    port = 256*secondlast+last;
}

/* Returns 0 if succeded the authentification, -1 if error */ 
void authentificate(int socket, char recvBuff[], char sendBuff[]){
    int n = 0;
    logRead(socket, recvBuff);
 
    if (strncmp(recvBuff, "220", 3) == 0) {
        sprintf(sendBuff, "USER %s\r\n", gArgs.username);
        logWrite(socket, sendBuff);
    }
    logRead(socket, recvBuff);
    if (strncmp(recvBuff, "331", 3) == 0) {
        sprintf(sendBuff, "PASS %s\r\n", gArgs.password);
        logWrite(socket, sendBuff);
    }
    
    logRead(socket, recvBuff);
    
    if (strncmp(recvBuff, "230", 3) != 0) {
        pdie(2);
    }
}

void logRead(int socket, char recvBuff[]){
    int n;

    n = read(socket, recvBuff, BUFFER_SIZE-1);     
        
    if(n < 0) {
        printf("\n Read error \n");
        pdie(7);
    }
    
    recvBuff[n] = 0;

    printf("S->C: %s", recvBuff);

    if (gArgs.logfile != NULL) {
        char logtmp[100];
        sprintf(logtmp, "S->C: %s", recvBuff);
        fwrite(logtmp, 1, strlen(logtmp), gArgs.log);
    }
}

void logWrite(int socket, char sendBuff[]) {
    write(socket, sendBuff, strlen(sendBuff));
    printf("C->S: %s", sendBuff);


    if (gArgs.logfile != NULL) {
        char logtmp[100];
        sprintf(logtmp, "C->S: %s", sendBuff);
        fwrite(logtmp, 1, strlen(logtmp), gArgs.log);
    }
}

int connectSocket(int port) {
    struct hostent *he;
    int socket_fd = 0;
    struct sockaddr_in server_address; 

    memset(&server_address, '0', sizeof(server_address)); 

    if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error : Could not create socket \n");
        exit(0);
    } 

    if ((he = gethostbyname(gArgs.hostname)) == NULL) {
        puts("error resolving hostname..");
        exit(1);
    }

    memcpy(&server_address.sin_addr, he->h_addr_list[0], he->h_length);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port); 

    if( connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
       pdie(1);
    }

    return socket_fd;
}

void substrafter(char *out, char haystack[], char needle[], int nr) {
    int i;
    char *tmp;

    tmp = strstr(haystack , needle);
    for (i = 0; i < nr - 1; i++) {
        tmp = strstr(&tmp[1], needle);
    }
    strcpy(out, &tmp[1]);
}

int findBytes(char haystack[]) {
    char b[50];
    char e[50];

    substrafter(b, haystack, "(", 1);
    substrafter(e, b, " ", 1);

    b[strlen(b)-strlen(e)-1] = 0;
    return atoi(b);
}

void retriveFile(char sendBuff[], char recvBuff[], int control_socket){
    int filesocket = 0, bytesToDownload = 0, received = 0, n;
    unsigned char fileRecv[1024];
    FILE *p = NULL;

    memset(fileRecv, 0, sizeof(fileRecv));

    if (strncmp(recvBuff, "200", 3) == 0) {
    // Mode successfully set 
    }

    if (gArgs.active) {
        char portStr[40];
        int connectSocket = openServerSocket();

        /* Write portsting and recieve 200 Port successful */
        portString(portStr, connectSocket);
        logWrite(control_socket, portStr);
        logRead(control_socket, recvBuff);

        /* CONNECTION ERROR */
        if (strncmp(recvBuff, "200", 3) < 0) {
            pdie(1);
        }

        sprintf(sendBuff, "RETR %s\r\n",gArgs.downloadFile);
        logWrite(control_socket, sendBuff);
    
    /* SPENNENDE FEIL!! om den neste linja er her, kommer det en 426 Failure writing network stream, 
    ** om den ikke er der, sender den ikke feilmelding hvis fila den skal hente ikke eksisterer   
    */ 
    //    logRead(control_socket, recvBuff);
        
        /* FILE NOT FOUND */
        if (strncmp(recvBuff, "550", 3) == 0) {
            pdie(3);
        }

        filesocket = connectToMessageSocket(connectSocket);
        logRead(control_socket, recvBuff);

    } else {
        /* PASV mode */
        int port;
        strcpy(sendBuff, "PASV\r\n");
        logWrite(control_socket, sendBuff);
        logRead(control_socket, recvBuff);
        if (strncmp(recvBuff, "227", 3) < 0){
            /* Cannot enter PASV mode */
            pdie(1);
        }

        port = findPasvPort(recvBuff);
        filesocket = connectSocket(port);
        sprintf(sendBuff, "RETR %s\r\n",gArgs.downloadFile);
        logWrite(control_socket, sendBuff);
        logRead(control_socket, recvBuff);

        /* FILE NOT FOUND */
        if (strncmp(recvBuff, "550", 3) == 0) {
            pdie(3);
        }
    }
    
    bytesToDownload = findBytes(recvBuff);

    p = fopen(gArgs.downloadFile, "w");
    if (p== NULL) {
        printf("Error in opening a file..", gArgs.downloadFile);
    }

    while (received < bytesToDownload) {
        n = read(filesocket, fileRecv, BUFFER_SIZE-1);;
        
        fwrite(fileRecv, 1, n, p);

        received += n;
    }
   
    fclose(p); 
}


/* Sets ascii or binary mode */
void setType(int control_socket, char sendBuff[]){

    if (strcmp(gArgs.mode, "binary") == 0 ) {
        strcpy(sendBuff, "TYPE I\r\n");
    } else if (strcmp(gArgs.mode, "ASCII") == 0) {
        strcpy(sendBuff, "TYPE A\r\n");
    }
    logWrite(control_socket, sendBuff);
}
 
/* port string */
void portString(char out[], int connectSocket) {
    char hostn[400]; //placeholder for the hostname
    char tmp[5];
    struct hostent *hostIP; //placeholder for the IP address
    struct sockaddr_in tmpSock;
    int i, port;
    int tmpSockLen;

    /* get port nr */
    tmpSockLen = sizeof(tmpSock);
    getsockname(connectSocket, (struct sockaddr *) &tmpSock, &tmpSockLen);
    port = ntohs(tmpSock.sin_port);

    if(gethostname(hostn, sizeof(hostn)) == 0) {
        hostIP = gethostbyname(hostn); //the netdb.h function gethostbyname
        sprintf(hostn, "%s", inet_ntoa(*(struct in_addr *)hostIP->h_addr));
        
        for (i = 0; i < strlen(hostn); i++) {
            if (hostn[i] == '.')
                hostn[i] = ',';
        }
        sprintf(out, "PORT %s,%d,%d\r\n", hostn, port/256, port%256);
    }else {
        pdie(2);
    }
}


int openServerSocket() {
    int socket_fd;
    int message_socket;
    struct sockaddr_in server_address;
    
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    bzero ((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(0);

    if (bind(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address))){
        pdie(1);
    }
    return socket_fd;

}

int connectToMessageSocket(int connectSocket){
    int clientLen, message_socket;
    struct sockaddr_in client_address;   /* socket struct for client connection */

    listen(connectSocket, 5);
    clientLen = sizeof(client_address);
    if ((message_socket = accept(connectSocket, (struct sockaddr *) &client_address, &clientLen)) < 0){
        pdie(1);
    }

    return message_socket;

}



