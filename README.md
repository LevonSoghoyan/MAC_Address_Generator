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
    server_name 10.101.0.6;
    return 301 https://$host$request_uri;

	}
server {
        listen 443 ssl;
        server_name 10.101.0.6;

        ssl_certificate /etc/ssl/certs/nginx-selfsigned.crt;
        ssl_certificate_key /etc/ssl/private/nginx-selfsigned.key;

        ssl_protocols TLSv1.2 TLSv1.3;
        ssl_ciphers HIGH:!aNULL:!MD5;

        location / {
                root /var/www/html/mac_portal;
                index index.html;
                try_files $uri $uri/ =404;
        }

        location /gmac {
                proxy_pass http://127.0.0.1:55555;
                proxy_set_header Host $host;
                proxy_set_header X-Real-IP $remote_addr;
                proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
                proxy_set_header X-Forwarded-Proto $scheme;
        }

        location /force_gen {
                proxy_pass http://127.0.0.1:55555;
                proxy_set_header Host $host;
                proxy_set_header X-Real-IP $remote_addr;
                proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
                proxy_set_header X-Forwarded-Proto $scheme;
        }
        location /usedmac {
                proxy_pass http://127.0.0.1:55555;
                proxy_set_header Host $host;
                proxy_set_header X-Real-IP $remote_addr;
                proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
                proxy_set_header X-Forwarded-Proto $scheme;
        }
        location /showmac {
                proxy_pass http://127.0.0.1:55555;
                proxy_set_header Host $host;
                proxy_set_header X-Real-IP $remote_addr;
                proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
                proxy_set_header X-Forwarded-Proto $scheme;
        }
        location /find_active {
                proxy_pass http://127.0.0.1:55555;
                proxy_set_header X-Real-IP $remote_addr;
        }

        location /find_used {
                proxy_pass http://127.0.0.1:55555;
                proxy_set_header X-Real-IP $remote_addr;
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
