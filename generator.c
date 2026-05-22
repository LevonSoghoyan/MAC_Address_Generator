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
#include <sqlite3.h>

/*MACROS*/
#define PORT 55555
#define BUFF_SIZE 2048
#define CLIENTS_COUNT 5
#define MAX_HISTORY   100
#define DATA "Mac.db"
#define DATABASE "txt.txt"
#define PROMPT "Generator CLI> "

/*Global variables*/
char Shell_Command[BUFF_SIZE];
struct sockaddr_in address;
socklen_t add_size = sizeof(address);
int client_fd;
int opt = 1;
int* history_bound;
char* IP;
char cli_ip[BUFF_SIZE];
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
    printf("Server is listen\n");
    *Listen = 1;
}

void Server_Down()
{
    printf("Server is not listeingn\n");
    *Listen = 0;
}

void clients()
{
    printf("%-6s %-10s %-10s %-24s\n", "ID", "SOCKET", "PID", "CREATED-ON");
    int id = 0;
    for(int i = 0; i < CLIENTS_COUNT; i++) {
        id++;
        if (Clients[i].socket != 0) {
            printf("%-6d %-10d %-10d %-24s\n", id, Clients[i].socket, Clients[i].pid, Clients[i].time);
        }
    }
}

void Status()
{
    printf("%-6s %-8s %-16s\n", "ID", "PORT", "IP");
    int id = 0;
    for(int i = 0; i < CLIENTS_COUNT; i++) {
        id++;
        if (Clients[i].socket != 0) {
            printf("%-6d %-8d %-16s\n", id, Clients[i].port, Clients[i].ip);
        }
    }
}

void ps()
{
    printf("%-10s %-20s %-15s\n", "PID", "COMMAND", "TIME");
    for(int i = 0; i < CLIENTS_COUNT; i++) {
        if (Processes[i].pid != 0) {
            printf("%-10d %-20s %-15ld seconds\n", Processes[i].pid, Processes[i].command, Processes[i].start - (*Server_Time));
        }
    }
}

void history()
{
    int curr_indx = *history_bound;
    curr_indx ++;
    printf("%-10s  %-30s  %-30s \n", "PID", "COMMAND", "RESULT");

    do
    {
        if (History[curr_indx].pid != 0)
            printf("%-10d  %-30s  %.30s \n", History[curr_indx].pid, History[curr_indx].command, History[curr_indx].result);
        curr_indx++;
        curr_indx %= MAX_HISTORY;
    }
    while (curr_indx != (*history_bound));
}

void Remove_Client(int new_client)
{

    for(int i = 0; i < CLIENTS_COUNT; i++) {

        if (Clients[i].socket == new_client) {
            Clients[i].socket = 0;
            strcpy(History[(*history_bound)].result,"Disconnected");
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

void Add_Process(int pid, char* command) 
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

void MAC_Generator(const char* S_N) 
{
    sqlite3* DB;
    int exit = sqlite3_open(DATA, &DB);
    char* err = 0;
    char message[BUFF_SIZE * 2];

    if (exit != SQLITE_OK) {
        snprintf(message, 200, "Filed to open database:\n");
        send(client_fd, message, strlen(message), 0);
        return;
    }

    char *sql_table = "CREATE TABLE IF NOT EXISTS MAC(IP TEXT,SERIAL_NUMBER TEXT,MAC_ADDRESS TEXT);";
    exit = sqlite3_exec(DB, sql_table, NULL, 0, &err);
    if (exit != SQLITE_OK) {
        snprintf(message, 200, "SQL Error creating table: %s\n", err);
        send(client_fd, message, strlen(message), 0);
        sqlite3_free(err);
        return;
    }
    /*Chacking device registration*/

    int found = 0;
    sqlite3_stmt *stmt1;
    sqlite3_stmt *stmt2;
    sqlite3_stmt *stmt3;

    sql_table = "SELECT EXISTS(SELECT 1 FROM MAC where SERIAL_NUMBER = ? );";
    sqlite3_prepare_v2(DB, sql_table, -1, &stmt1, NULL);
    sqlite3_bind_text(stmt1, 1, S_N, -1, SQLITE_TRANSIENT); 
    if (sqlite3_step(stmt1) == SQLITE_ROW) {
        if (sqlite3_column_int(stmt1, 0) > 0) {
            found = 1;
        }
    }
    sqlite3_finalize(stmt1);

    if (!found) {
        /*Genereting MAC address*/
        unsigned char hash[SHA256_DIGEST_LENGTH];
        unsigned char MAC[6];
        char mac[BUFF_SIZE];

        SHA256((const unsigned char*)S_N, strlen(S_N), hash);
        for (int i = 0; i < 6; i++) {
            MAC[i] = hash[i];
        }

        MAC[0] &= 0XFE;
        MAC[0] |= 0X02;

        snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);

        sql_table = "INSERT INTO MAC VALUES(?,?,?);";
        sqlite3_prepare_v2(DB, sql_table, -1, &stmt2, NULL);
        

        sqlite3_bind_text(stmt2, 1, cli_ip, -1, SQLITE_TRANSIENT); 
        sqlite3_bind_text(stmt2, 2, S_N, -1, SQLITE_TRANSIENT); 
        sqlite3_bind_text(stmt2, 3, mac, -1, SQLITE_TRANSIENT); 
        sqlite3_step(stmt2);
        sqlite3_finalize(stmt2);

        sql_table = "SELECT * FROM MAC where SERIAL_NUMBER = ?";
        sqlite3_prepare_v2(DB, sql_table, -1, &stmt3, NULL);
        sqlite3_bind_text(stmt3, 1, S_N, -1, SQLITE_TRANSIENT); 
        if (sqlite3_step(stmt3) == SQLITE_ROW) {
            snprintf(message, 2000, "Device registered successfully. IP: %-30s | SN: %-30s | MAC: %-30s\n",
                    sqlite3_column_text(stmt3,0),
                    sqlite3_column_text(stmt3,1),
                    sqlite3_column_text(stmt3,2));
            send(client_fd, message, strlen(message), 0);
        }
        sqlite3_finalize(stmt3);

    } else {

        sql_table = "SELECT * FROM MAC where SERIAL_NUMBER = ?";
        sqlite3_prepare_v2(DB, sql_table, -1, &stmt3, NULL);
        sqlite3_bind_text(stmt3, 1, S_N, -1, SQLITE_TRANSIENT); 
        if (sqlite3_step(stmt3) == SQLITE_ROW) {
            snprintf(message, 2000, "Device already registered. IP: %-30s | SN: %-30s | MAC: %-30s\n",
                    sqlite3_column_text(stmt3,0),
                    sqlite3_column_text(stmt3,1),
                    sqlite3_column_text(stmt3,2));
            send(client_fd, message, strlen(message), 0);
        }
        sqlite3_finalize(stmt3);
    }
    sqlite3_close(DB);
}

void Client_Devices(char* Client_IP)
{

    sqlite3* DB;
    int exit = sqlite3_open(DATA, &DB);
    char message[BUFF_SIZE * 2];
    sqlite3_stmt *stmt3;

    if (exit != SQLITE_OK) {
        snprintf(message, 200, "Filed to open database:\n");
        send(client_fd, message, strlen(message), 0);
        send(client_fd, "NULL", 5, 0);
        return;
    }

    char* err = 0;
    char *sql_table = "SELECT * FROM MAC where IP = ?";
    sqlite3_prepare_v2(DB, sql_table, -1, &stmt3, NULL);
    sqlite3_bind_text(stmt3, 1, cli_ip, -1, SQLITE_TRANSIENT); 

    while (sqlite3_step(stmt3) == SQLITE_ROW) {
        snprintf(message, 2000, "Registered Device. IP: %-30s | SN: %-30s | MAC: %-30s\n",
                sqlite3_column_text(stmt3,0),
                sqlite3_column_text(stmt3,1),
                sqlite3_column_text(stmt3,2));
        send(client_fd, message, strlen(message), 0);
    }

    send(client_fd, "NULL", 5, 0);
    sqlite3_finalize(stmt3);
    sqlite3_close(DB);
}

void Client_Command()
{
    memset(Shell_Command, 0, BUFF_SIZE);
    int bytes = recv(client_fd , Shell_Command, BUFF_SIZE - 1, 0);

    Shell_Command[bytes] = '\0';
    Shell_Command[strcspn(Shell_Command, "\n")] = '\0';

    if (strncmp(Shell_Command, "shell", 5) == 0) {
        char Formatted_Command[BUFF_SIZE * 2];
        char buffer[BUFF_SIZE];

        memset(buffer, 0, BUFF_SIZE);
        
        char* pcmd = Shell_Command + 6;
        pcmd[strcspn(pcmd, "\n")] = '\0';

        History[(*history_bound)].pid =  getpid();
        strcpy(History[(*history_bound)].command,pcmd);

        snprintf(Formatted_Command, sizeof(Formatted_Command), "%s 2>&1", pcmd);
        Add_Process(getpid(), pcmd);

        FILE* pf;
        pf = popen(Formatted_Command, "r");       
        while(fgets(buffer, sizeof(buffer), pf) != NULL) {

            send(client_fd, buffer, strlen(buffer), 0);
            buffer[strcspn(buffer, "\n")] = ';';

            strcat(History[(*history_bound)].result,buffer);
            memset(buffer, 0, BUFF_SIZE);
        }

        (*history_bound) ++;
        (*history_bound) %= MAX_HISTORY;

        strcpy(buffer, "NULL");
        send(client_fd, buffer, strlen(buffer), 0);

        pclose(pf);
        Remove_Processe(getpid());
    } else if (strcmp(Shell_Command, "disconnect") == 0) {

        History[(*history_bound)].pid =  getpid();
        strcpy(History[(*history_bound)].command,Shell_Command);
        Remove_Client(client_fd);
        
        (*history_bound) ++ ;
        (*history_bound) %= MAX_HISTORY;

        close(client_fd);
        exit(0);
    } else if (strncmp(Shell_Command, "gmac", 4) == 0) {
        unsigned char* S_N  = Shell_Command + 5;
        MAC_Generator(S_N);
    } else if (strncmp(Shell_Command, "showmac", 7) == 0) {
        unsigned char* Client_IP  = Shell_Command + 5;
        Client_Devices(Client_IP);
    }
}

void Server_CLI()
{
    char Server_Command[BUFF_SIZE];

    while(1) {
        char * input = readline(PROMPT);
        strncpy(Server_Command, input, BUFF_SIZE - 1);
       
        int No_Alpha = 0;
        while (No_Alpha < strlen(Server_Command) && isalnum(Server_Command[No_Alpha]) == 0) 
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
    
    History[(*history_bound)].pid =  getpid();
    strcpy(History[(*history_bound)].command, "Connect");
    strcpy(History[(*history_bound)].result, "Rejected");

    struct sockaddr_in add;
    int res = getpeername(new_client, (struct sockaddr *)&add, &add_size);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &add.sin_addr, ip, INET_ADDRSTRLEN);
    strcpy(cli_ip,ip);
    for (int i = 0; i < CLIENTS_COUNT; i++) {
        if (Clients[i].socket == 0) {
            (*C_Count)++;
            Clients[i].socket = new_client;
            time_t Ctime;
            time(&Ctime);
            strncpy(Clients[i].time, ctime(&Ctime), sizeof(Clients[i].time));
            Clients[i].port = ntohs(add.sin_port);
            Clients[i].pid = getpid();
            strcpy(Clients[i].ip, ip);
            strcpy(History[(*history_bound)].result, "Connected");
            if (*Listen == 1)
                printf("\nClient connected. Port: %u IP: %s\n", ntohs(add.sin_port), ip);
            break;
        }
    }
    (*history_bound)++;
    (*history_bound) %= MAX_HISTORY;
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

    history_bound = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memset(history_bound, 0, sizeof(int));

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
            printf("Invalid IP address. Try again \n");
            IP = (char*) malloc(30 * sizeof(char));
            scanf("%28s", IP);
        }
    } else {
        IP = "127.0.0.1";
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
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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
                setpgid(0, pgid);
                Add_Client(client_fd);
                close(server_fd);

                while (1) {

                    Client_Command();
                }
                exit(0);

            } else if (new_pid > 0) {
                setpgid(new_pid, pgid);
                close(client_fd);
                continue;

            } else {
                printf("Error: %s \n", strerror(errno));
                return 0;

            }
        }
}

