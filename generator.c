/*INCLUDES*/ 
#include <time.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <ctype.h>
#include <signal.h>
#include "openssl/sha.h"
#include <arpa/inet.h>
#include <netinet/in.h>

/*MACROS*/
#define PORT 8080
#define BUFF_SIZE 2048
#define CLIENTS_COUNT 5
#define MAX_HISTORY   100
#define DATABASE "txt.txt"
#define LINE_LENGHT  100
#define PROMPT "Server> "

/*Global variables*/
char Shell_Command[BUFF_SIZE];
struct sockaddr_in address;
socklen_t add_size = sizeof(address);
int client_fd;
int opt = 1;
int* history_baund;
char* IP;
int* C_Count;
int* Listen;
int *Server_Time;
/*Structs*/
typedef struct Clients {
    int socket;
    char ip[INET_ADDRSTRLEN];
    int port;
    int pid;
    char time[30];
} Tclients;

Tclients *Clients;

typedef struct {
    int pid;
    char command[BUFF_SIZE];
    char result[BUFF_SIZE];
} S_History;

S_History *History;

typedef struct {
    int pid;
    char command[BUFF_SIZE];
    time_t start;
} PS;

PS *Processes;
void Server_Up()
{
    printf("Server is Listen\n");
    *Listen = 1;
}

void Server_Down()
{
    printf("Server dont Listen\n");
    *Listen = 0;
}

void clients()
{
    printf("ID       SOCKET     PID             CREATED-ON\n");
    int id = 0;
    for(int i = 0; i < CLIENTS_COUNT; i++) {
        id++;
        if (Clients[i].socket != 0) {
            printf("%d        %d          %d         %s", id, Clients[i].socket, Clients[i].pid, Clients[i].time);
        }
    }
}

void Status()
{
    printf("ID       PORT           IP\n");
    int id = 0;
    for(int i = 0; i < CLIENTS_COUNT; i++) {
        id++;
        if (Clients[i].socket != 0) {
            printf("%d        %d          %s\n", id, Clients[i].port, Clients[i].ip);
        }
    }
}

void ps()
{
    printf("PID       COMMAND     TIME\n");
    for(int i = 0; i < CLIENTS_COUNT; i++) {
        if (Processes[i].pid != 0) {
            printf("%d        %s          %ld second\n", Processes[i].pid, Processes[i].command, Processes[i].start - (*Server_Time));
        }
    }
}

void history()
{
    int curr_indx = *history_baund;
    curr_indx ++;
    printf("%-10s  %-30s  %-30s \n", "PID", "COMMAND", "RESULT");

    do
    {
        if (History[curr_indx].pid != 0)
            printf("%-10d  %-30s  %.30s \n", History[curr_indx].pid, History[curr_indx].command, History[curr_indx].result);
        curr_indx++;
        curr_indx %= MAX_HISTORY;
    }
    while (curr_indx != (*history_baund));
}

void Remove_Client(int new_client)
{

    for(int i = 0; i < CLIENTS_COUNT; i++) {

        if (Clients[i].socket == new_client) {
            Clients[i].socket = 0;
            strcmp(History[(*history_baund)].result,"Disconnected");
            (*C_Count)--;
            break;
        }
    }
}

void Remove_Processe(int pid) 
{

    for(int i = 0; i < CLIENTS_COUNT; i++) {
        if (Processes[i].pid == pid) {
            Processes[i].pid = 0;
            break;
        }
    }
}

void Add_Processe(int pid, char* command) 
{

    for(int i = 0; i < CLIENTS_COUNT; i++) {
        if (Processes[i].pid == 0) {
            Processes[i].pid = pid;
            strcpy(Processes[i].command, command);
            time(&Processes[i].start);
            break;
        }
    }
}

void MAC_Generator(const char* S_N) {
    FILE *pFile = fopen(DATABASE, "r");
    if (!pFile) {
        printf("File opening error\n");
        return;
    }

    char *pLine = NULL;
    size_t len = 0;
    int found = 0;
    char *Serial_num;

    while (getline(&pLine, &len, pFile) != -1) {
        Serial_num = strtok(pLine, " ");
        if (strcmp(Serial_num, S_N) == 0) {
            found = 1;
            break; 
        }
    }

    fclose(pFile);
    char message[BUFF_SIZE];

    if (!found) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256((const unsigned char*)S_N, strlen(S_N), hash);

        unsigned char MAC[6];
        for (int i = 0; i < 6; i++) {
            MAC[i] = hash[i];
        }

        FILE *pAppend = fopen(DATABASE, "a");
        if (!pAppend) {
            printf("File opening error for appending\n");
            return;
        }

        fprintf(pAppend, "%s %02X:%02X:%02X:%02X:%02X:%02X\n", S_N, MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
        snprintf(message, 200, "Serial Number %s registered in database whith MAC addres %02X:%02X:%02X:%02X:%02X:%02X .\n", S_N, MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
        send(client_fd, message, strlen(message), 0);
        fclose(pAppend);
    } else {
        Serial_num = strtok(NULL, "\n");
        snprintf(message, 200, "Serial Number %s alrady registered in database whith MAC addres %s .\n", S_N, Serial_num);
        send(client_fd, message, strlen(message), 0);
    }
    free(pLine);
}

void Client_Command()
{
    memset(Shell_Command, 0, BUFF_SIZE);
    int bytes = recv(client_fd , Shell_Command, BUFF_SIZE - 1, 0);

    Shell_Command[bytes] = '\0';
    Shell_Command[strcspn(Shell_Command, "\n")] = '\0';

    if (strncmp(Shell_Command, "shell", 5) == 0) {
        char Formated_Command[BUFF_SIZE * 2];
        char buffer[BUFF_SIZE];

        memset(buffer, 0, BUFF_SIZE);
        
        char* pcmd = Shell_Command + 6;
        pcmd[strcspn(pcmd, "\n")] = '\0';

        History[(*history_baund)].pid =  getpid();
        strcpy(History[(*history_baund)].command,pcmd);

        snprintf(Formated_Command, sizeof(Formated_Command), "%s 2>&1", pcmd);
        Add_Processe(getpid(), pcmd);

        FILE* pf;
        pf = popen(Formated_Command, "r");       
        while(fgets(buffer, sizeof(buffer), pf) != NULL) {

            send(client_fd, buffer, strlen(buffer), 0);
            buffer[strcspn(buffer, "\n")] = ';';

            strcat(History[(*history_baund)].result,buffer);
            memset(buffer, 0, BUFF_SIZE);
        }

        (*history_baund) ++;
        (*history_baund) %= MAX_HISTORY;

        strcpy(buffer, "NULL");
        send(client_fd, buffer, strlen(buffer), 0);

        pclose(pf);
        Remove_Processe(getpid());
    } else if (strcmp(Shell_Command, "disconnect") == 0) {

        History[(*history_baund)].pid =  getpid();
        strcpy(History[(*history_baund)].command,Shell_Command);
        Remove_Client(client_fd);
        
        (*history_baund) ++ ;
        (*history_baund) %= MAX_HISTORY;

        close(client_fd);
        exit(0);
    } else if (strcmp(Shell_Command, "gmac") == 0) {
        unsigned char* S_N  = Shell_Command + 5;
        MAC_Generator(S_N);
    }
}

void Server_CLI()
{
    char Server_Command[BUFF_SIZE];

    while(1) {
        char * input = readline(PROMPT);
        strncpy(Server_Command, input, BUFF_SIZE - 1);
       
        int No_Alpha = 0;
        while (No_Alpha < strlen(Server_Command) && isalpha(Server_Command[No_Alpha]) == 0) 
            No_Alpha++;

        for (int i = 0; i < BUFF_SIZE - No_Alpha; i++)
            Server_Command[i] = Server_Command[i + No_Alpha];

        Server_Command[strcspn(Server_Command, "\n")] = '\0';
        
        if (strlen(Server_Command) <= 1)
            continue;

        add_history(Server_Command);

        if (strcmp(Server_Command, "up") == 0) {
            Server_Up();
        } else if (strcmp(Server_Command, "down") == 0) {
            Server_Down();
        } else if (strcmp(Server_Command, "clients") == 0) {
            clients();
        } else if (strcmp(Server_Command, "ps") == 0) {
            ps();
        } else if (strcmp(Server_Command, "history") == 0) {
            history();
        } else if (strcmp(Server_Command, "status") == 0) {
            Status();
        } else {
            printf("Usage:\n");
            printf("COMMAND                    DESCRIPTION\n");
            printf("up                         Start server\n"); 
            printf("down                       Stop server\n");
            printf("clients                    Show connection list\n");
            printf("status                     Show connection status\n");
            printf("ps                         Show process list\n");
            printf("history                    History\n");
        }
    }
}

void Add_Client(int new_client)
{
    struct sockaddr_in add;
    History[(*history_baund)].pid =  getpid();
    strcpy(History[(*history_baund)].command, "Connect");
    strcpy(History[(*history_baund)].result, "Rejected");

    int res = getpeername(new_client, (struct sockaddr *)&add, &add_size);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &add.sin_addr, ip, INET_ADDRSTRLEN);
    for (int i = 0; i < CLIENTS_COUNT; i++) {
        if (Clients[i].socket == 0) {
            (*C_Count)++;
            Clients[i].socket = new_client;
            time_t Ctime;
            time(&Ctime);
            strncpy(Clients[i].time, ctime(&Ctime), sizeof(Clients[i].time));
            Clients[i].port = ntohs(add.sin_port);
            Clients[i].pid = getpid();
            strcpy(Clients[i].ip, IP);
            strcpy(History[(*history_baund)].result, "Connected");
            if (*Listen == 1)
                printf("\nClient connected. Port: %u IP: %s\n", ntohs(add.sin_port), ip);
            break;
        }
    }
    (*history_baund)++;
    (*history_baund) %= MAX_HISTORY;
}

void handle(int sig)
{

    for (int i = 0; i < CLIENTS_COUNT; i++) {

        if (Clients[i].pid != 0) {
            char command[BUFF_SIZE] = "kill -9 ";
            snprintf(command + strlen(command),sizeof(command) - 1, "%d", Clients[i].pid);
            system(command);
            close(Clients[i].socket);
        }
    }

    exit(0);
}

int main(int argc, char* argv[])
{
    pid_t pgid;
    setpgid(0, 0);
    pgid = getpid();
    /*Shered Memoey*/
    Clients = mmap(NULL, sizeof(Tclients) * CLIENTS_COUNT, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memset(Clients, 0, sizeof(Tclients) * CLIENTS_COUNT);

    Processes = mmap(NULL, sizeof(PS) * CLIENTS_COUNT, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memset(Processes, 0, sizeof(PS) * CLIENTS_COUNT);

    History = mmap(NULL, sizeof(S_History) * MAX_HISTORY, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memset(History, 0, sizeof(S_History) * MAX_HISTORY);

    history_baund = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memset(history_baund, 0, sizeof(int));

    Listen = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memset(Listen, 0, sizeof(int));

    C_Count = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memset(C_Count, 0, sizeof(int));

    Server_Time = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memset(Server_Time, 0, sizeof(int));
    time_t t;
    time(&t);
    *Server_Time = t;
    /*IP addres chacking*/
    if (argc > 1) {
        IP = argv[1];
        while (!inet_pton(AF_INET, IP, &(address.sin_addr))) {
            printf("Invalid IP address try again \n");
            IP = (char*) malloc(30 * sizeof(char));
            scanf("%28s", IP);
        }
    } else {
        IP = "0.0.0.0";
    }
    pid_t pid = fork();

    if (pid == 0)
    {
        Server_CLI();
        exit(0);
    }

    signal(SIGINT,handle);

    printf("\nSERVER CLI\n");

    /*Creating server socket*/
    int server_fd;

        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("Error: %s \n", strerror(errno));
            return 0;
        }

        /*Adding server config*/
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(IP);
        address.sin_port = htons(PORT);

        /* Making server socket to listening mode*/
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            printf("Error: %s \n", strerror(errno));
            return 0;
        }
        if (listen(server_fd, 3) < 0) {
            printf("Error: %s \n", strerror(errno));
        }


        while (1) {

            struct sockaddr_in client_address;

            client_fd = accept(server_fd, (struct sockaddr*)&client_address, &add_size);

            if (*C_Count == CLIENTS_COUNT) {
                send(client_fd, "SERVER FULL", 11, 0);
            } else if (*Listen == 0) {
                send(client_fd, "SERVER DOWN", 11, 0);
            } else {
                send(client_fd, "SERVER   UP", 11, 0);
            }

            pid_t new_pid = fork();

            if (new_pid == 0) {

                Add_Client(client_fd);
                setpgid(0, pgid);
                close(server_fd);

                while (1) {

                    Client_Command();
                }
                exit(0);

            } else if (new_pid > 0) {
                setpgid(new_pid, pgid);
                continue;

            } else {
                printf("Error: %s \n", strerror(errno));
                return 0;

            }
        }
}

