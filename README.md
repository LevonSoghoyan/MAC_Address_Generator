---
  MAC ADDRESS GENERATOR
---

Default IP Endpoint: 10.1.0.102


---
STEP 1: INSTALL OPERATING PLATFORM PACKAGES
---

$ sudo apt update

$ sudo apt install gcc libsqlite3-dev libssl-dev nginx openssl ufw coreutils -y

---
STEP 2: BUILD THE HARDENED APPS ENGINE
---

$ gcc server.c -o mac_server -lcrypto -lsqlite3

---
STEP 3: EXECUTE THE BACKGROUND MICROSERVICE
---

$ ./mac_server

---
STEP 4: GENERATE THE SSL CERTIFICATE SIGNATURES
---

$ sudo openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -keyout /etc/ssl/private/nginx-selfsigned.key \
    -out /etc/ssl/certs/nginx-selfsigned.crt

---
STEP 5: SECURE THE PLATFORM FIREWALLS
---

$sudo ufw allow 443/tcp

$sudo ufw reload

---
STEP 6: UPDATE THE PROXY HOSTER DEFINE BLOCKS
---

$ sudo vim /etc/nginx/sites-available/default

---
---

Wipe old placeholder entries completely and paste this proxy architecture map:




	server {
    listen 80;
    listen [::]:80;

    server_name 10.1.0.102;

    root /var/www/html;
    index index.html;

    access_log /var/log/nginx/mac_gen_access.log;
    error_log /var/log/nginx/mac_gen_error.log;

    location / {
        try_files $uri $uri/ =404;
    }

    location ~ ^/(gmac|force_gen|showmac|usedmac|find_active|find_used) {
        proxy_pass http://127.0.0.1:55555;

        proxy_set_header Authorization "";

        proxy_set_header X-Real-EMAIL $http_x_real_email;

        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        add_header 'Access-Control-Allow-Origin' '*' always;
        add_header 'Access-Control-Allow-Methods' 'GET, POST, OPTIONS' always;
        add_header 'Access-Control-Allow-Headers' 'DNT,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Range,X-Real-EMAIL' always;

        if ($request_method = 'OPTIONS') {
            add_header 'Access-Control-Allow-Origin' '*';
            add_header 'Access-Control-Allow-Methods' 'GET, POST, OPTIONS';
            add_header 'Access-Control-Allow-Headers' 'DNT,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Range,X-Real-EMAIL';
            add_header 'Access-Control-Max-Age' 1728000;
            add_header 'Content-Type' 'text/plain; charset=utf-8';
            add_header 'Content-Length' 0;
            return 204;
        }
        proxy_connect_timeout 90s;
        proxy_send_timeout 90s;
        proxy_read_timeout 90s;
    }
}


---
STEP 7: RUN SYNTAX VALIDATIONS AND RESTART PROXY
---

$sudo nginx -t

$ sudo systemctl restart nginx

---
STEP 8: RUN SERVER
---


$sudo mkdir -p /var/www/html/mac_portal

$sudo chmod +x rebuild

./rebuild

---
 Command-Line Editing via sqlite3
 ---

sqlite3 mac_database.db

.tables

.schema mac_records

<....>

.exit


---
Graphical Editing via sqlitebrowser
---

sqlitebrowser mac_database.db 

---
