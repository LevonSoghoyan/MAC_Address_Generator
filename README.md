=============================================================================
         PROJECT: BARCODE SCANNER & MAC ADDRESS GENERATOR SYSTEM
=============================================================================

1. PROJECT OVERVIEW
-----------------------------------------------------------------------------
This is a C-based networked system designed for hardware provisioning and device 
registration. The architecture connects distributed barcode scanner clients to a 
central generator engine via robust TCP network sockets.

Core Logic Flow:
- Scanner (Client) captures or reads a device hardware Serial Number.
- The Serial Number is transmitted directly over the network to the Generator.
- Generator (Server) inspects a local storage engine (Mac.db) for the key.
- If it exists: Returns known allocation profile data (Serial, MAC, IP).
- If it DOES NOT exist: Deterministically hashes the Serial Number string,
  re-formats the hash into a valid IEEE 802 standard MAC address, updates the
  Mac.db storage tables, and returns the newly registered profile to the node.

2. SYSTEM COMPONENTS & FILES
-----------------------------------------------------------------------------
- generator.c    : Server application managing socket listeners, SQLite database 
                   connections, hashing mechanics, and administrative shells.
- scanner.c      : Client application modeling hardware interfaces, CLI consoles, 
                   and remote data dispatch layers.
- Makefile       : Automated compilation instructions for independent binaries.

3. COMPILATION
-----------------------------------------------------------------------------
Use the explicit make targets below inside your workspace terminal:

- To compile the Generator:
  $ make generator

- To compile the Scanner Application:
  $ make scanner

- To remove temporary binary outputs and clean workspace:
  $ make clean

4. GENERATOR COMMANDS (Interactive execution shell)
-----------------------------------------------------------------------------
COMMAND      DESCRIPTION
up           Initialize network sockets, bind handles, and start listening.
down         Stop listener engines and cleanly flush system states.
clients      Enumerate a live list of all scanner connections on network.
status       Show global connection state, socket details, and uptime health.
ps           Display active native processes running on the host system.
history      Display an operational chronological history log of actions.

5. SCANNER COMMANDS (Interactive terminal utility)
-----------------------------------------------------------------------------
COMMAND             DESCRIPTION
connect <IP>        Establish a reliable TCP socket connection to a host at <IP>.
gmac                Open up the local Barcode Scanner entry CLI loop.
showmac             Query and present previously logged units from this node.
shell <command>     Pass a remote shell instruction directly onto the host.
status              Verify network status and check active latency layers.
disconnect          Gracefully close connections to the generator system.
exit                Terminate and quit the local application loop.

6. DATA STORAGE LOGIC (Mac.db)
-----------------------------------------------------------------------------
- Database file name: Mac.db
- Behavioral mechanics: The generator inspects its execution path for Mac.db 
  on boot. If the database file is missing, the engine creates it seamlessly 
  and initializes internal index schema.
- Data tracked: [Device Serial Number] | [Calculated MAC] | [Scanner Node IP]

7. STEP-BY-STEP QUICK START WORKFLOW
-----------------------------------------------------------------------------
Step 1: Build components
        $ make generator 
        $ make scanner

Step 2: Fire up the Generator engine node:
        $ ./GENERATOR 127.0.0.1
        Generator CLI > up

Step 3: Access Scanner console from terminal instance B:
        $ ./SCANNER
        Client CLI > connect 127.0.0.1

Step 4: Execute Scanner operations:
        Scanner Console > gmac
        Scaner CLI > 2134-123
        Registered Device. IP: 10.101.0.6                     | SN: 2134-123                       | MAC: 46:F0:82:9A:5F:6B
=============================================================================
