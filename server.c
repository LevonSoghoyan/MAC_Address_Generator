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
#define DATA "db_files/Mac.db"
#define USED_MAC_ADDRESSES "db_files/Used_Mac.db"

// Initialize both databases and verify progressive seed tracker row
void Init_Database() {
    sqlite3* USED;
    char* err_msg = 0;
    if (sqlite3_open(USED_MAC_ADDRESSES, &USED) == SQLITE_OK) {
        char* sql = "CREATE TABLE IF NOT EXISTS USED("
                    "IP TEXT, "
                    "SERIAL_NUMBER TEXT, "
                    "PART_NUMBER TEXT, "
                    "MAC_ADDRESS TEXT);";
        sqlite3_exec(USED, sql, 0, 0, &err_msg);
        sqlite3_close(USED);
    }

    sqlite3* DB;
    err_msg = 0;
    if (sqlite3_open(DATA, &DB) == SQLITE_OK) {
        char* sql = "CREATE TABLE IF NOT EXISTS MAC("
                    "IP TEXT, "
                    "SERIAL_NUMBER TEXT, "
                    "PART_NUMBER TEXT, "
                    "MAC_ADDRESS TEXT UNIQUE);";
        sqlite3_exec(DB, sql, 0, 0, &err_msg);

        sqlite3_stmt *stmt;
        int tracking_row_exists = 0;
        if (sqlite3_prepare_v2(DB, "SELECT 1 FROM MAC WHERE IP='0' AND SERIAL_NUMBER='0';", -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                tracking_row_exists = 1;
            }
            sqlite3_finalize(stmt);
        }

        if (!tracking_row_exists) {
            sqlite3_exec(DB, "INSERT INTO MAC VALUES('0', '0', '0', '1');", 0, 0, &err_msg);
        }
        sqlite3_close(DB);
    }
}

// Extract real client IP from Nginx downstream headers
void Extract_Real_IP(const char* request, char* output_ip) {
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

// Sequential MAC generator with duplicate look-up protection
void MAC_Generator_Web(int sock, const char* S_N, const char* P_N, int count, const char* remote_ip) {
    sqlite3* DB;
    if (sqlite3_open(DATA, &DB) != SQLITE_OK) {
        char* err = "HTTP/1.1 500 Error\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nDB Failure.";
        send(sock, err, strlen(err), 0);
        return;
    }

    char message[BUFF_SIZE * 10] = "";
    char line_msg[512];
    int found_existing = 0;

    sqlite3_stmt *stmt_find;
    char *sql_find = "SELECT MAC_ADDRESS FROM MAC WHERE SERIAL_NUMBER = ? AND PART_NUMBER = ? AND IP != '0';";
    if (sqlite3_prepare_v2(DB, sql_find, -1, &stmt_find, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt_find, 1, S_N, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt_find, 2, P_N, -1, SQLITE_TRANSIENT);

        while (sqlite3_step(stmt_find) == SQLITE_ROW) {
            found_existing = 1;
            const char* existing_mac = (const char*)sqlite3_column_text(stmt_find, 0);
            snprintf(line_msg, sizeof(line_msg), "IP: %s | PN: %s | SN: %s | MAC: %s\n", remote_ip, P_N, S_N, existing_mac);
            strcat(message, line_msg);
        }
        sqlite3_finalize(stmt_find);
    }

    if (!found_existing) {
        long long current_counter = 1;
        sqlite3_stmt *stmt_get;
        if (sqlite3_prepare_v2(DB, "SELECT MAC_ADDRESS FROM MAC WHERE IP='0' AND SERIAL_NUMBER='0';", -1, &stmt_get, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt_get) == SQLITE_ROW) {
                current_counter = atoll((const char*)sqlite3_column_text(stmt_get, 0));
            }
            sqlite3_finalize(stmt_get);
        }

        for (int i = 0; i < count; i++) {
            unsigned char MAC[6];
            char mac_str[18];

            MAC[0] = 0XA0;
            MAC[1] = 0X24;
            MAC[2] = 0X90;
            MAC[3] = (current_counter >> 16) & 0xFF;
            MAC[4] = (current_counter >> 8) & 0xFF;
            MAC[5] = current_counter & 0xFF;
            MAC[3] &= 0X0F;
            MAC[3] |= 0XB0;

            snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                     MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);

            sqlite3_stmt *stmt_ins;
            char *sql_ins = "INSERT INTO MAC VALUES(?,?,?,?);";
            if (sqlite3_prepare_v2(DB, sql_ins, -1, &stmt_ins, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt_ins, 1, remote_ip, -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt_ins, 2, S_N, -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt_ins, 3, P_N, -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt_ins, 4, mac_str, -1, SQLITE_TRANSIENT);
                sqlite3_step(stmt_ins);
                sqlite3_finalize(stmt_ins);
            }

            snprintf(line_msg, sizeof(line_msg), "IP: %s | PN: %s | SN: %s | MAC: %s\n", remote_ip, P_N, S_N, mac_str);
            strcat(message, line_msg);
            current_counter++;
        }

        sqlite3_stmt *stmt_upd;
        char next_counter_str[32];
        snprintf(next_counter_str, sizeof(next_counter_str), "%lld", current_counter);

        if (sqlite3_prepare_v2(DB, "UPDATE MAC SET MAC_ADDRESS = ? WHERE IP='0' AND SERIAL_NUMBER='0';", -1, &stmt_upd, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt_upd, 1, next_counter_str, -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt_upd);
            sqlite3_finalize(stmt_upd);
        }
    }

    char http_response[BUFF_SIZE * 12];
    snprintf(http_response, sizeof(http_response),
             "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s",
             strlen(message), message);
    send(sock, http_response, strlen(http_response), 0);
    sqlite3_close(DB);
}

// Archives existing allocations to Used_Mac.db and generates using the NEW target count value
void Force_Generating(int sock, const char* S_N, const char* P_N, int new_count, const char* remote_ip) {
    sqlite3* DB;
    if (sqlite3_open(DATA, &DB) != SQLITE_OK) {
        char* err = "HTTP/1.1 500 Error\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nDB Failure.";
        send(sock, err, strlen(err), 0);
        return;
    }

    sqlite3* USED;
    if (sqlite3_open(USED_MAC_ADDRESSES, &USED) != SQLITE_OK) {
        char* err = "HTTP/1.1 500 Error\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nUSED DB Failure.";
        send(sock, err, strlen(err), 0);
        sqlite3_close(DB);
        return;
    }

    sqlite3_stmt *stmt_sel;
    char *sql_sel = "SELECT IP, SERIAL_NUMBER, PART_NUMBER, MAC_ADDRESS FROM MAC WHERE SERIAL_NUMBER = ? AND PART_NUMBER = ? AND IP != '0';";
    if (sqlite3_prepare_v2(DB, sql_sel, -1, &stmt_sel, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt_sel, 1, S_N, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt_sel, 2, P_N, -1, SQLITE_TRANSIENT);

        while (sqlite3_step(stmt_sel) == SQLITE_ROW) {
            sqlite3_stmt *stmt_arch;
            char *sql_arch = "INSERT INTO USED VALUES(?,?,?,?);";
            if (sqlite3_prepare_v2(USED, sql_arch, -1, &stmt_arch, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt_arch, 1, (const char*)sqlite3_column_text(stmt_sel, 0), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt_arch, 2, (const char*)sqlite3_column_text(stmt_sel, 1), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt_arch, 3, (const char*)sqlite3_column_text(stmt_sel, 2), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt_arch, 4, (const char*)sqlite3_column_text(stmt_sel, 3), -1, SQLITE_TRANSIENT);
                sqlite3_step(stmt_arch);
                sqlite3_finalize(stmt_arch);
            }
        }
        sqlite3_finalize(stmt_sel);
    }
    sqlite3_close(USED);

    sqlite3_stmt *stmt_del;
    char *sql_del = "DELETE FROM MAC WHERE SERIAL_NUMBER = ? AND PART_NUMBER = ? AND IP != '0';";
    if (sqlite3_prepare_v2(DB, sql_del, -1, &stmt_del, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt_del, 1, S_N, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt_del, 2, P_N, -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt_del);
        sqlite3_finalize(stmt_del);
    }
    sqlite3_close(DB);

    MAC_Generator_Web(sock, S_N, P_N, new_count, remote_ip);
}

// Dump workstation data tables with explicit cast-safety
void Client_Devices_Web(int sock, const char* ip) {
    sqlite3* DB;
    char row[256];
    char body[BUFF_SIZE * 10] = "";
    char http_response[BUFF_SIZE * 12] = "";
    sqlite3_stmt *stmt;

    if (sqlite3_open(DATA, &DB) != SQLITE_OK) {
        char* err = "HTTP/1.1 500 Error\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nDB Failure.";
        send(sock, err, strlen(err), 0);
        return;
    }

    char *sql_select = "SELECT IP, SERIAL_NUMBER, PART_NUMBER, MAC_ADDRESS FROM MAC WHERE IP == ? AND SERIAL_NUMBER != '0' ORDER BY ROWID DESC;";
    if (sqlite3_prepare_v2(DB, sql_select, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, ip, -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            snprintf(row, sizeof(row), "IP: %s | PN: %s | SN: %s | MAC: %s\n",
                     (const char*)sqlite3_column_text(stmt, 0),
                     (const char*)sqlite3_column_text(stmt, 2),
                     (const char*)sqlite3_column_text(stmt, 1),
                     (const char*)sqlite3_column_text(stmt, 3));
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

// Dynamic parameter lookup tool tracking active database matches strictly
void find_device_active(int sock, const char* S_N, const char* P_N) {
    sqlite3* DB;
    char row[256];
    char body[BUFF_SIZE * 10] = "";
    char http_response[BUFF_SIZE * 12] = "";
    sqlite3_stmt *stmt;

    if (sqlite3_open(DATA, &DB) != SQLITE_OK) {
        char* err = "HTTP/1.1 500 Error\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nDB Failure.";
        send(sock, err, strlen(err), 0);
        return;
    }

    char *sql_select = "SELECT IP, SERIAL_NUMBER, PART_NUMBER, MAC_ADDRESS FROM MAC "
                       "WHERE IP != '0' AND (?1 = '' OR SERIAL_NUMBER = ?1) AND (?2 = '' OR PART_NUMBER = ?2) ORDER BY ROWID DESC;";

    if (sqlite3_prepare_v2(DB, sql_select, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, S_N, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, P_N, -1, SQLITE_TRANSIENT);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            snprintf(row, sizeof(row), "IP: %s | PN: %s | SN: %s | MAC: %s\n",
                     (const char*)sqlite3_column_text(stmt, 0),
                     (const char*)sqlite3_column_text(stmt, 2),
                     (const char*)sqlite3_column_text(stmt, 1),
                     (const char*)sqlite3_column_text(stmt, 3));
            strcat(body, row);
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(DB);

    snprintf(http_response, sizeof(http_response),
             "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s",
             strlen(body), body);
    send(sock, http_response, strlen(http_response), 0);
}

// Dynamic parameter lookup tool tracking archived log partition matches strictly
void find_device_used(int sock, const char* S_N, const char* P_N) {
    sqlite3* USED;
    char row[256];
    char body[BUFF_SIZE * 10] = "";
    char http_response[BUFF_SIZE * 12] = "";
    sqlite3_stmt *stmt;

    if (sqlite3_open(USED_MAC_ADDRESSES, &USED) != SQLITE_OK) {
        char* err = "HTTP/1.1 500 Error\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nUSED DB Failure.";
        send(sock, err, strlen(err), 0);
        return;
    }

    char *sql_select = "SELECT IP, SERIAL_NUMBER, PART_NUMBER, MAC_ADDRESS FROM USED "
                       "WHERE (?1 = '' OR SERIAL_NUMBER = ?1) AND (?2 = '' OR PART_NUMBER = ?2) ORDER BY ROWID DESC;";

    if (sqlite3_prepare_v2(USED, sql_select, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, S_N, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, P_N, -1, SQLITE_TRANSIENT);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            snprintf(row, sizeof(row), "IP: %s | PN: %s | SN: %s | MAC: %s\n",
                     (const char*)sqlite3_column_text(stmt, 0),
                     (const char*)sqlite3_column_text(stmt, 2),
                     (const char*)sqlite3_column_text(stmt, 1),
                     (const char*)sqlite3_column_text(stmt, 3));
            strcat(body, row);
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(USED);

    snprintf(http_response, sizeof(http_response),
             "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s",
             strlen(body), body);
    send(sock, http_response, strlen(http_response), 0);
}

// Dump historical archived partitions out of Used_Mac.db
void Client_used_MAC(int sock) {
    sqlite3* USED;
    char row[256];
    char body[BUFF_SIZE * 10] = "";
    char http_response[BUFF_SIZE * 12] = "";
    sqlite3_stmt *stmt;

    if (sqlite3_open(USED_MAC_ADDRESSES, &USED) != SQLITE_OK) {
        char* err = "HTTP/1.1 500 Error\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nUSED Failure.";
        send(sock, err, strlen(err), 0);
        return;
    }

    char *sql_select = "SELECT IP, SERIAL_NUMBER, PART_NUMBER, MAC_ADDRESS FROM USED ORDER BY ROWID DESC;";
    if (sqlite3_prepare_v2(USED, sql_select, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            snprintf(row, sizeof(row), "IP: %s | PN: %s | SN: %s | MAC: %s\n",
                     (const char*)sqlite3_column_text(stmt, 0),
                     (const char*)sqlite3_column_text(stmt, 2),
                     (const char*)sqlite3_column_text(stmt, 1),
                     (const char*)sqlite3_column_text(stmt, 3));
            strcat(body, row);
        }
        sqlite3_finalize(stmt);
    }

    snprintf(http_response, sizeof(http_response),
             "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s",
             strlen(body), body);
    send(sock, http_response, strlen(http_response), 0);
    sqlite3_close(USED);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addr_size = sizeof(address);

    Init_Database();

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket allocation failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Socket binding broken");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 50) < 0) {
        perror("Listen state broken");
        exit(EXIT_FAILURE);
    }


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
                Extract_Real_IP(buffer, web_client_ip);

                char extracted_sn[100] = {0};
                char* sn_start = strstr(buffer, "sn=");
                if (sn_start) {
                    sn_start += 3;
                    int idx = 0;
                    while (sn_start[idx] != ' ' && sn_start[idx] != '&' && sn_start[idx] != '\0' && idx < 99) {
                        extracted_sn[idx] = sn_start[idx];
                        idx++;
                    }
                    extracted_sn[idx] = '\0';
                }

                char extracted_pn[100] = {0};
                char* pn_start = strstr(buffer, "pn=");
                if (pn_start) {
                    pn_start += 3;
                    int idx = 0;
                    while (pn_start[idx] != ' ' && pn_start[idx] != '&' && pn_start[idx] != '\0' && idx < 99) {
                        extracted_pn[idx] = pn_start[idx];
                        idx++;
                    }
                    extracted_pn[idx] = '\0';
                }

                int count_val = 1;
                char* count_start = strstr(buffer, "count=");
                if (count_start) {
                    count_start += 6;
                    char count_str[20] = {0};
                    int idx = 0;
                    while (count_start[idx] != ' ' && count_start[idx] != '&' && count_start[idx] != '\0' && idx < 19) {
                        count_str[idx] = count_start[idx];
                        idx++;
                    }
                    count_str[idx] = '\0';
                    count_val = atoi(count_str);
                }

                // CRITICAL CAPACITY BOUNDARY REJECTION INTERCEPTOR
                if (count_val > 50 || count_val < 1) {
                    char* validation_err = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nError: Requested count bounds exceeded. Maximum allocation per individual pipeline transaction is capped at 50 to maintain database stability.\n";
                    send(client_fd, validation_err, strlen(validation_err), 0);
                }
                else if (strncmp(buffer, "GET /gmac", 9) == 0) {
                    if (strlen(extracted_sn) == 0 || strlen(extracted_pn) == 0) {
                        char* validation_err = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nError: Both Serial Number (SN) and Part Number (PN) are strictly required parameters.\n";
                        send(client_fd, validation_err, strlen(validation_err), 0);
                    } else {
                        MAC_Generator_Web(client_fd, extracted_sn, extracted_pn, count_val, web_client_ip);
                    }
                }
                else if (strncmp(buffer, "GET /force_gen", 14) == 0) {
                    if (strlen(extracted_sn) == 0 || strlen(extracted_pn) == 0) {
                        char* validation_err = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nError: Both input metrics are required to execute overwrite tasks.\n";
                        send(client_fd, validation_err, strlen(validation_err), 0);
                    } else {
                        Force_Generating(client_fd, extracted_sn, extracted_pn, count_val, web_client_ip);
                    }
                }
                else if (strncmp(buffer, "GET /showmac", 12) == 0) {
                    Client_Devices_Web(client_fd, web_client_ip);
                }
                else if (strncmp(buffer, "GET /usedmac", 12) == 0) {
                    Client_used_MAC(client_fd);
                }
                else if (strncmp(buffer, "GET /find_active", 16) == 0) {
                    if (strlen(extracted_sn) == 0 && strlen(extracted_pn) == 0) {
                        char* validation_err = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nError: Search criteria missing.\n";
                        send(client_fd, validation_err, strlen(validation_err), 0);
                    } else {
                        find_device_active(client_fd, extracted_sn, extracted_pn);
                    }
                }
                else if (strncmp(buffer, "GET /find_used", 14) == 0) {
                    if (strlen(extracted_sn) == 0 && strlen(extracted_pn) == 0) {
                        char* validation_err = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nError: Search criteria missing.\n";
                        send(client_fd, validation_err, strlen(validation_err), 0);
                    } else {
                        find_device_used(client_fd, extracted_sn, extracted_pn);
                    }
                }
            }
            close(client_fd);
            exit(0);
        }
        close(client_fd);
    }
    return 0;
}
