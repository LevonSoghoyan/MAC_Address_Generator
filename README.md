-------------------------------------------------------------------------------
 DEPLOYMENT MANIFEST: HIGH-SPEED MAC ADDRESS PROVISIONING PORTAL
-------------------------------------------------------------------------------

File Type: Code-Free Deployment Guide, Architecture Blueprint & Operations Manual

Security Layer: HTTPS / TLS 1.3 via Nginx SSL Termination

Target Environment: Isolated Production Network / Workstation Cluster

Default IP Endpoint: 10.101.0.6

-------------------------------------------------------------------------------
1. MODERNIZED WEB ARCHITECTURE BLUEPRINT
-------------------------------------------------------------------------------

By dropping support for the legacy terminal/CLI socket clients, the network 
engine transforms from a restrictive connection manager into a lightweight, 
high-performance HTTP microservice. 

Key Architectural Improvements:

* CONCURRENT PROCESS ISOLATION: The server core bypasses old sequential execution 
  queues by utilizing background forks for stateless web transactions. Each 
  request spawns a temporary worker process that executes database changes 
  independently and terminates instantly, freeing up 100% of the core system 
  memory buffers.

* UNLIMITED VERTICAL SCALING: The legacy hardcoded boundary limiting connections 
  to 5 active physical scanner clients is completely eliminated. Because web 
  browsers drop the connection the exact millisecond data packets are received, 
  the server can seamlessly process simultaneous data entries from hundreds of 
  technicians across the network.

* AUTHENTIC ENDPOINT TRACKING: When data is dispatched through the web interface, 
  the background engine does not log the local proxy loopback signature 
  (127.0.0.1). Instead, it inspects incoming headers to grab the true 'X-Real-IP' 
  token injected by Nginx, registering the precise physical network address of 
  the technician's scanning terminal.

-------------------------------------------------------------------------------
2. HIGH-SPEED PRODUCTION LINE WORKFLOW
-------------------------------------------------------------------------------

The user interface dashboard is structurally stripped of visual clutter and 
engineered specifically to keep pace with continuous hardware barcode gun loops.

* STAGE 1: AUTOMATIC FOCUS LOCK
  The moment the workstation page initializes, the typing cursor automatically 
  activates and begins flashing inside the serial number input element. No mouse 
  interaction or initial clicking is required.

* STAGE 2: INSTANT SUBMISSION
  A physical barcode gun automatically appends an [ENTER] keypress command upon 
  reading a product tag. The webpage natively catches this form submission, 
  ships the raw data string to the backend silently over an AJAX promise, and 
  injects the new generated parameters into the active on-screen history grid.

* STAGE 3: INFINITE CAPTURE LOOP
  As soon as the server transaction clears, the frontend form intercepts the event, 
  wipes out the text box field completely, and immediately forces the blinking 
  typing cursor back inside the field. The technician can rapidly scan dozens of 
  incoming hardware components in a row without breaking physical rhythm.

-------------------------------------------------------------------------------
3. STEP-BY-STEP TERMINAL DEPLOYMENT MANUAL
-------------------------------------------------------------------------------

Follow this explicit terminal execution sequence to initialize dependencies, 
build your optimized server execution file, and wrap your portal in an 
encrypted HTTPS data pipeline.

-------------------------------------------------------------------------------
STEP 1: INSTALL OPERATING PLATFORM PACKAGES
-------------------------------------------------------------------------------

Run your Linux package update manager to verify and install essential compilation 
tools, cryptographic math handles, proxy proxies, and database drivers:

$ sudo apt update

$ sudo apt install gcc libsqlite3-dev libssl-dev nginx openssl ufw coreutils -y

-------------------------------------------------------------------------------
STEP 2: BUILD THE HARDENED APPS ENGINE
-------------------------------------------------------------------------------

Because the interactive shell CLI inputs have been cleaned from the backend, 
there is no longer a requirement to map terminal handles. Compile your clean 
engine by linking standard OpenSSL and SQLite handles:
$ gcc server.c -o mac_server -lcrypto -lsqlite3

-------------------------------------------------------------------------------
STEP 3: EXECUTE THE BACKGROUND MICROSERVICE
-------------------------------------------------------------------------------

Launch the clean binary engine directly. It boots into the background, maps 
database boundaries, and listens internally over port 55555:

$ ./mac_server

-------------------------------------------------------------------------------
STEP 4: GENERATE THE SSL CERTIFICATE SIGNATURES
-------------------------------------------------------------------------------

Because this workstation environment operates inside a private network cluster 
IP (10.101.0.6), public token issuers cannot verify ownership. Issue a secure 
self-signed SSL certificate valid for 365 days directly on the machine:

$ sudo openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -keyout /etc/ssl/private/nginx-selfsigned.key \
    -out /etc/ssl/certs/nginx-selfsigned.crt

(Note: You can skip the detailed regional questionnaires by tapping [ENTER] 
through each field prompt to save the system default parameters).

-------------------------------------------------------------------------------
STEP 5: SECURE THE PLATFORM FIREWALLS
-------------------------------------------------------------------------------

Ensure your local network protection rules allow external encrypted data streams 
to clear ports managed by Nginx:

$sudo ufw allow 443/tcp$ sudo ufw reload

-------------------------------------------------------------------------------
STEP 6: UPDATE THE PROXY HOSTER DEFINE BLOCKS
-------------------------------------------------------------------------------

Open your central Nginx reverse proxy routing profiles file:
$ sudo nano /etc/nginx/sites-available/default

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

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
        proxy_set_header X-Real-IP $remote_addr;
    }
    location /showmac {
        proxy_pass http://127.0.0.1:55555;
        proxy_set_header X-Real-IP $remote_addr;
    }
    
}

-------------------------------------------------------------------------------
STEP 7: RUN SYNTAX VALIDATIONS AND RESTART PROXY
-------------------------------------------------------------------------------
Validate your Nginx configuration files for structural formatting errors and 
reload your system server daemons to pull changes live:
$sudo nginx -t$ sudo systemctl restart nginx

-------------------------------------------------------------------------------
STEP 8: LOAD DASHBOARD ASSETS
-------------------------------------------------------------------------------
Configure your deployment root folder paths and load the web frontend pages 
into the location managed by the Nginx proxy routing layer:
$sudo mkdir -p /var/www/html/mac_portal$ sudo cp index.html /var/www/html/mac_portal/index.html

-------------------------------------------------------------------------------
4. PRODUCTION WORKSTATION MANUAL
-------------------------------------------------------------------------------

1. Turn on a terminal or laptop connected to the localized assembly network 
   and open a web browser to the secure address: https://10.101.0.6/

2. Bypass your browser's default self-signed signature warning screen (This notice 
   is normal and expected on internal private networks):
   -> Click on the "Advanced" button in the lower left corner of the block.
   -> Select the link "Proceed to 10.101.0.6 (unsafe)".

3. The graphical dashboard initializes immediately over an encrypted HTTPS pipeline.

4. Aim your physical scanner tool at a hardware device serial identifier tag and 
   pull the gun trigger. The registry layout logs will compute and map live entries 
   with perfect continuous focusing rhythm.
   
-------------------------------------------------------------------------------
