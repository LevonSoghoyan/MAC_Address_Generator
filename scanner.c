/*INCLUDES*/
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

/*MACROS*/
#define PORT 55555
#define BUFF_SIZE 2048

#define PROMPT "Client CLI> "
#define SPROMPT "Scanner CLI> "
/*Global variables*/
char buf[BUFF_SIZE];
int client_fd = -1;
struct sockaddr_in server_addr;
socklen_t addr_len = sizeof(server_addr);

void Status()
{
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in addr;

    if (getpeername(client_fd, (struct sockaddr *)&addr, &addr_len) == 0)
        inet_ntop(AF_INET, &addr.sin_addr, ip, INET_ADDRSTRLEN);
    if (client_fd && inet_pton(AF_INET, ip, &(addr.sin_addr)) == 1) {
        printf("Client connected to server with the IP address %s \n", ip);
    } else {
        printf("Client disconnected \n");
    }
}

void Deactivate();

void Connect(char* IP)
{
    /*Creating Socket*/
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error: %s \n",strerror(errno));
    }
    if (inet_pton(AF_INET, IP, &server_addr.sin_addr) < 0) {
        printf("Error: %s \n",strerror(errno));
    }

    /*Connecting client whit server*/
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Error: %s\n",strerror(errno));
    }

    memset(buf, 0, BUFF_SIZE);
    recv(client_fd, buf, 11, 0);

    if (strcmp(buf,"SERVER DOWN") == 0) {
        Deactivate("SERVER DOWN");
    } else if (strcmp(buf,"SERVER FULL") == 0) {
        Deactivate("SERVER FULL");
    } else if (strcmp(buf,"SERVER STOP") == 0) {
        Deactivate("SERVER STOP");
    } else
        Status();  

    return;
}

void Shell(char* command)
{

    memset(buf, 0, BUFF_SIZE);
    if (command == "disconnect")
    {
        send(client_fd, command, BUFF_SIZE, 0);
        return;
    } else if (strncmp(command, "gmac", 4) == 0) {
        printf("Scanner Mode\n");
        printf("Type 'exit' to close the CLI\n");
        while(1) {
            char* input = readline(SPROMPT);
            strncpy(buf, input, BUFF_SIZE -1);   

            int No_Alpha = 0;
            while (No_Alpha < strlen(buf) && isalnum(buf[No_Alpha]) == 0) 
                No_Alpha++;

            for (int i = 0; i < BUFF_SIZE - No_Alpha; i++)
                buf[i] = buf[i + No_Alpha];

            if (strlen(buf) <= 1)
                continue;

            add_history(buf);

            if (strcmp(buf,"exit") == 0)
                break;
            char message[BUFF_SIZE *2];
            snprintf(message, sizeof(message), "gmac %s\n", buf);
            send(client_fd, message, BUFF_SIZE, 0);
            memset(buf, 0, BUFF_SIZE);
            recv(client_fd, buf, BUFF_SIZE, 0);
            printf("%s\n", buf);
        }
        return;
    }

    send(client_fd, command, BUFF_SIZE, 0);
    recv(client_fd, buf, BUFF_SIZE, 0);

    while(strcmp(buf,"NULL") != 0)
    {
        printf("%s", buf);
        memset(buf, 0, BUFF_SIZE);
        recv(client_fd, buf, BUFF_SIZE, 0);
    }
    return;
}

void Deactivate(char* message)
{
    char* command = "disconnect";
    Shell(command);
    printf("%s\n", message);
    close(client_fd);
    client_fd = -1;
}
void D_connect()
{
    char* command = "disconnect";
    Shell(command);

    printf("Disconnected\n");
    close(client_fd);
    client_fd = -1;
}

void handle(int sig)
{
    printf("SIGINT\n");
    char* command = "disconnect";
    Shell(command);
    send(client_fd,"SIGINT",6,0);
    close(client_fd);
    client_fd = -1;
}
int main(int argc, char* argv[])
{
    printf("\nCLIENT CLI\n");

    /*Adding server configurations*/
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    char buf[BUFF_SIZE];

    while(1) {

        char* input = readline(PROMPT);
        strncpy(buf, input, BUFF_SIZE -1);   

        signal(SIGINT,handle);

        int No_Alpha = 0;
        while (No_Alpha < strlen(buf) && isalnum(buf[No_Alpha]) == 0) 
            No_Alpha++;


        for (int i = 0; i < BUFF_SIZE - No_Alpha; i++)
            buf[i] = buf[i + No_Alpha];

        if (strlen(buf) <= 1)
            continue;

        add_history(buf);

        char *token;
        token = strtok(buf, " ");
        token[strcspn(token,"\n")] = '\0';

        if (strcmp(token, "connect") == 0) {
            if (client_fd != -1)
                printf("Client already connected\n");
            else
                Connect((buf + strlen(token) + 1));
        } else if (!client_fd) {
            printf("Client disconnected\n");
        } else if (strcmp(token, "gmac") == 0) {
            Shell(buf);
        } else if (strcmp(token, "showmac") == 0) {
            Shell(buf);
        } else if (strcmp(token, "shell") == 0) {
            Shell(buf);
        } else if (strcmp(token, "disconnect") == 0) {
            D_connect();
        } else if (strcmp(token, "status") == 0) {
            Status();
        } else if (strcmp(token, "exit") == 0) {
            break;
        } else {
            printf("Usage:\n");
            printf("COMMAND                     DESCRIPTION\n");
            printf("connect  <IP>               Connect to server at <IP>\n");
            printf("shell <command>             Run <command> in the server terminal\n");
            printf("disconnect                  Disconnect the client \n");
            printf("status                      Show connection status\n");
            printf("gmac                        Open the scanner CLI\n");
            printf("showmac                     Show your registered devices\n");
            printf("exit                        Close the terminal\n");
        }
    }
}
