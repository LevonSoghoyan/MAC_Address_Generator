#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sqlite3.h>
#include <openssl/sha.h>
#include <errno.h>

#define PORT 55555
#define BUFF_SIZE 2048
#define DATA "Mac.db"

void Real_IP(const char* request, char* output_ip) {
    char* header = strstr(request, "X-Real-IP: ");
    if (header) {
        header += 11;
        int i = 0;
        while (header[i] != '\r' && header[i] != '\n' && header[i] != ' ' && i < 15) {
            output_ip[i] = header[i];
            i++;
        }
        output_ip[i] = '\0';
    } else {
        strcpy(output_ip, "127.0.0.1");
    }
}

void MAC_Generator(int sock, const char* S_N, const char* remote_ip) {
    sqlite3* DB;
    char* err_msg = 0;
    if (sqlite3_open(DATA, &DB) == SQLITE_OK) {
        char* sql = "CREATE TABLE IF NOT EXISTS MAC("
                    "IP TEXT, "
                    "SERIAL_NUMBER TEXT UNIQUE, "
                    "MAC_ADDRESS TEXT);";
        sqlite3_exec(DB, sql, 0, 0, &err_msg);
        sqlite3_close(DB);
    }
    char message[BUFF_SIZE] = {0};
    char http_response[BUFF_SIZE * 2] = {0};
    sqlite3_stmt *stmt;

    if (sqlite3_open(DATA, &DB) != SQLITE_OK) {
        char* err = "HTTP/1.1 500 Error\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nDB Failure.";
        send(sock, err, strlen(err), 0);
        return;
    }

    int found = 0;
    char *sql_select = "SELECT IP, MAC_ADDRESS FROM MAC WHERE SERIAL_NUMBER = ?;";
    if (sqlite3_prepare_v2(DB, sql_select, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, S_N, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            snprintf(message, sizeof(message), "Device already registered. IP: %s | SN: %s | MAC: %s\n",
                     sqlite3_column_text(stmt, 0), S_N, sqlite3_column_text(stmt, 1));
            found = 1;
        }
        sqlite3_finalize(stmt);
    }

    if (!found) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        unsigned char MAC[6];
        char mac_str[18];

        SHA256((const unsigned char*)S_N, strlen(S_N), hash);
        for (int i = 0; i < 6; i++) MAC[i] = hash[i];
        MAC[0] &= 0XFE; 
        MAC[0] |= 0X02; 

        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", 
                 MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);

        char *sql_insert = "INSERT INTO MAC VALUES(?,?,?);";
        if (sqlite3_prepare_v2(DB, sql_insert, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, remote_ip, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, S_N, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, mac_str, -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }

        snprintf(message, sizeof(message), "Device registered successfully. IP: %s | SN: %s | MAC: %s\n",
                 remote_ip, S_N, mac_str);
    }

    snprintf(http_response, sizeof(http_response),
             "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s",
             strlen(message), message);
    send(sock, http_response, strlen(http_response), 0);
    sqlite3_close(DB);
}

void Client_Devices(int sock, const char* ip) {
    sqlite3* DB;
    char row[256];
    char body[BUFF_SIZE * 4] = "";
    char http_response[BUFF_SIZE * 5] = "";
    sqlite3_stmt *stmt;

    if (sqlite3_open(DATA, &DB) != SQLITE_OK) {
        char* err = "HTTP/1.1 500 Error\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nDB Failure.";
        send(sock, err, strlen(err), 0);
        return;
    }

    char *sql_select = "SELECT IP, SERIAL_NUMBER, MAC_ADDRESS FROM MAC where IP = ?;";
    if (sqlite3_prepare_v2(DB, sql_select, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, ip, -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            snprintf(row, sizeof(row), "IP: %s | SN: %s | MAC: %s\n",
                     sqlite3_column_text(stmt, 0),
                     sqlite3_column_text(stmt, 1),
                     sqlite3_column_text(stmt, 2));
            strcat(body, row);
        }
        sqlite3_finalize(stmt);
    }

    snprintf(http_response, sizeof(http_response),
             "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s",
             strlen(body), body);
    send(sock, http_response, strlen(http_response), 0);
    sqlite3_close(DB);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addr_size = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket allocation failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Binding engine breakdown");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 50) < 0) {
        perror("Listening loop breakdown");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&address, &addr_size);
        if (client_fd < 0) continue;

        if (fork() == 0) {
            close(server_fd);

            char buffer[BUFF_SIZE] = {0};
            int bytes = recv(client_fd, buffer, BUFF_SIZE - 1, 0);
            if (bytes > 0) {
                buffer[bytes] = '\0';

                char web_client_ip[46] = {0};
                Real_IP(buffer, web_client_ip);

                if (strncmp(buffer, "GET /gmac", 9) == 0) {
                    char* sn_start = strstr(buffer, "sn=");
                    if (sn_start) {
                        sn_start += 3;
                        char extracted_sn[100] = {0};
                        int idx = 0;
                        while (sn_start[idx] != ' ' && sn_start[idx] != '&' && sn_start[idx] != '\0' && idx < 99) {
                            extracted_sn[idx] = sn_start[idx];
                            idx++;
                        }
                        extracted_sn[idx] = '\0';
                        MAC_Generator(client_fd, extracted_sn, web_client_ip);
                    }
                } 
                else if (strncmp(buffer, "GET /showmac", 12) == 0) {
                    Client_Devices(client_fd, web_client_ip);
                }
            }
            close(client_fd);
            exit(0); 
        }
        close(client_fd); 
    }
    return 0;
}
