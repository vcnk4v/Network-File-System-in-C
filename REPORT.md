# Network File System (NFS) Project

## Introduction

The NFS project is designed to provide a distributed file system where multiple clients can access and manage files stored on remote servers. The system comprises a central Naming Server (NS) responsible for managing the directory structure and maintaining essential information about file locations. Additionally, there are Storage Servers (SS) that store the actual file data. Clients interact with these servers to perform various file operations.

## Components

### 1. Naming Server (NS)

The Naming Server plays a crucial role in managing the directory structure and keeping track of file locations within the system. Key responsibilities include dynamic registration of Storage Servers, handling privileged file operations, and maintaining metadata for the entire file system.


### 2. Storage Server (SS)

Storage Servers are responsible for storing the actual file data. They register with the Naming Server to make their storage capacity available to clients. The SS also ensures fault tolerance by maintaining backup servers, thus preventing data loss in case of server failures.

#### Features:

- **Dynamic Registration:** SS can dynamically register with the NS, making it scalable and adaptable to changes in the server infrastructure.
- **Fault Tolerance:** Backup Storage Servers are employed to enhance system reliability. In the event of a primary SS failure, the backup takes over to ensure continuous file access.

### 3. Client

Clients are end-users or applications that interact with the NFS to perform file operations. They communicate with the NS for privileged operations and directly interact with SS for non-privileged operations like reading and writing files.

#### Features:

- **File Operations:** Clients can perform various file operations, including reading, writing, creating, and deleting files and folders.
- **Folder Copy:** Supports the copying of both individual files and entire folders. The NS facilitates this operation by coordinating with relevant SS.

## Command Format

- **Read:** `read <path>`
- **Write:** `write <path>`
- **Create File:** `create_file <path>`
- **Create Folder:** `create_folder <path>`
- **Delete File:** `delete_file <path>`
- **Delete Folder:** `delete_folder <path>`
- **Copy File:** `copy_file <path_from> <path_to>`
- **Copy Folder:** `copy_folder <path_from> <path_to>`
- **Get Size:** `get_size_per <path>`


## Interaction and Initialization

### System Initialization:
- The NFS system is initialized with a predefined set of Storage Servers (SS), each with its unique identifier, storage capacity, and network details. These servers are known to the Naming Server (NS) during the system setup.

### Dynamic Registration of Storage Servers:
- Storage Servers have the capability of dynamic registration with the Naming Server. This allows for scalability and adaptability to changes in the server infrastructure.
- When a new Storage Server comes online or is added to the system, it dynamically registers with the Naming Server by providing its details, such as identifier, storage capacity, and network information.

### Client Request to Naming Server:
- When a client intends to perform a file operation or access a specific file, it sends a request to the Naming Server.
- The client specifies the type of operation it wants to perform (e.g., read, write, create, delete) and provides the filename or directory path as part of the request.

### Naming Server's Role:
- The Naming Server, upon receiving the client's request, checks its directory structure and metadata to determine the location of the file or the appropriate Storage Server for the requested operation.
- If the operation is privileged (e.g., file creation, copying, deletion), the Naming Server coordinates and manages these operations on behalf of the client.

### Details of Storage Server:
- For non-privileged file operations (e.g., reading, writing), the Naming Server provides the client with the details (such as network location) of the relevant Storage Server where the file is stored or where the operation can be performed.
- The client then directly interacts with the identified Storage Server for the non-privileged operation.

### Communication Between Client and Storage Server:
- The client establishes a direct communication channel with the identified Storage Server to perform the requested operation.
- For reading a file, the client retrieves the file content from the Storage Server, and for writing, it sends the new data to the Storage Server for storage.

### Fault Tolerance:
- In the event of a primary Storage Server failure, the backup Storage Servers play a crucial role in maintaining system reliability. The Naming Server redirects clients to the backup Storage Server, ensuring continuous file access and operation execution.

## Reading, Writing, and Retrieving Information about Files:

**Client Request:**
Clients initiate file-related operations by sending requests to the Naming Server. For instance, a request like `READ dir1/dir2/file.txt` is passed to the NM.

**NM Facilitation:**
The NM takes on the responsibility of locating the correct Storage Server (SS) where the file is stored. It looks over all accessible paths in SS1, SS2, SS3, and so on. Once the NM identifies the correct SS (let's call it SSx), it returns precise IP address and client port information for that SS to the client.

**Direct Communication:**
The client establishes direct communication with the designated SS (SSx). This communication channel is set up, allowing the client to continuously receive information packets from the SS until a predefined "STOP" packet is received or a specified condition for task completion is met. The "STOP" packet serves as a signal to conclude the operation.

## Creating and Deleting Files and Folders:

**Client Request:**
Clients request file and folder creation or deletion operations by providing the respective path and action (create or delete).

**NM Facilitation:**
The NM, upon receiving the request, determines the correct SS for the operation and forwards the request to the appropriate SS for execution.

**SS Execution:**
The identified SS processes the request and performs the specified action, such as creating an empty file or deleting a file/folder.

**Acknowledgment and Feedback:**
After successful execution, the SS sends an acknowledgment (ACK) or a STOP packet to the NM to confirm task completion. The NM conveys this information back to the client, providing feedback on the task’s status.

## Copying Files/Directories Between Storage Servers:

**Client Request:**
Clients can request to copy files or directories between SS by providing both the source and destination paths.

**NM Execution:**
The NM, upon receiving the copy request, assumes responsibility for managing the file/folder transfer between the relevant SS. It determines the source and destination SS for the copy operation.

**NM Orchestrates Copying Process:**
The NM orchestrates the copying process, ensuring that data is transferred accurately. This may involve copying data from one SS to another, even if both source and destination paths lie within the same SS.

**Acknowledgment and Client Notification:**
Upon successful completion of the copy operation, the NM sends an acknowledgment (ACK) packet to the client. This ACK serves as confirmation that the requested data transfer has been successfully executed.

## Multiple Clients


### Client Port Assignment:
- Clients connect to the Naming Server (NM) port.
- NM assigns a separate port to each client from an array of 20 ports, assuming a maximum of 20 clients.
- Each port has a dedicated thread to handle the client's requests.

### Initial ACK Mechanism:
- NM responds to client requests with an initial ACK to acknowledge the receipt of the request.
- If the initial ACK is not received within a reasonable timeframe, the client may display a timeout message.


## Concurrent File Reading:

### Storage Server (SS) Structure:
- Each Storage Server (SS) contains a structure with the following attributes:
  - Filepath
  - Reader count
  - Readers lock
  - Writers lock
- Additionally, read and write semaphores are initialized, with the write semaphore set to 1.

### Read and Write Operations:
- Multiple clients can read the same file simultaneously.
- Only one client can execute write operations on a file at any given time.

### Semaphore Usage:
- When a client requests to write, it uses semaphore `sem_wait` to acquire the write lock.
- If another client wants to write, a `sem_trywait` mechanism is implemented, prompting the second client to list down additional operations it wants to perform.
- This ensures that write operations are mutually exclusive, preventing multiple clients from writing to the same file simultaneously.


## Search in Naming Server

### Efficient Searching with Tries


- **Optimizing Search:** When a client initiates a request, such as reading or writing a file, the NS employs Tries to traverse the directory structure efficiently. Instead of performing a linear search across all Storage Servers (SS), the NS can quickly identify the specific SS associated with the requested file by navigating the trie.

- **Scalability:** Tries offer scalability, making them suitable for systems with a large number of files and folders. As the directory structure grows, the trie allows for a logarithmic search time, ensuring that the performance of the system remains efficient.

### LRU Caching

- **Caching Recent Searches:** The NS maintains a cache with a predefined size of 10 entries. Each entry in the cache corresponds to a recent search operation, storing information about the file path and the associated SS details.

- **Cache Hit:** When a client initiates a request, the NS first checks its cache. If the requested file path is present in the cache (a cache hit), the NS can directly retrieve the SS details from the cache. This eliminates the need for an extensive search operation across all SS, significantly reducing response times.

- **Cache Miss:** In case of a cache miss (the requested file path is not in the cache), the NS resorts to the more comprehensive search using the trie data structure to determine the appropriate SS.

- **LRU Policy:** The cache follows the Least Recently Used (LRU) policy, ensuring that the most recently accessed entries are retained. If the cache reaches its capacity, older or less frequently accessed entries are replaced by new ones. This dynamic updating mechanism optimizes the use of cache space and ensures that relevant information is readily available.

By combining Tries for efficient directory traversal and LRU caching for recent searches, the NFS project enhances the overall responsiveness and efficiency of the Naming Server, particularly in scenarios with a substantial number of files and folders.

## Redundancy/Replication

### Failure Detection

- **Continuous Monitoring:** The NM continuously monitors Storage Server (SS) status for prompt failure detection, ensuring an effective response to disruptions in SS availability.

- **Regular Connectivity Checks:** Periodic messages or connection requests are sent by the NM to all registered SS, typically every 10 seconds, to establish a connection and verify responsiveness.

- **Failure Flagging:** If the NM fails to connect with a specific SS within the defined interval, it flags the SS as inactive or failed. A status indicator, often a flag set to zero, signifies the SS's current inactive state.

### SS Recovery

- **Flag Update:** The Naming Server updates the status flag of a previously inactive Storage Server (SS) to 1 when it becomes active again, signifying its operational state.

- **Socket Establishment:** Simultaneously, the Naming Server initiates the creation or reactivation of a communication socket for the recovered SS. This enables communication between the Naming Server and the now-active SS.

- **Message Notification:** Continuous messages from the Naming Server to all SS facilitate the detection of SS recovery. Successful connection establishment with a previously inactive SS is interpreted as recovery, prompting the Naming Server to update the SS status accordingly.

### Data Redundancy/Replication

- **Storage Redundancy:**
  - The system enhances data reliability by replicating an SS's contents across two additional backup SS, preventing data loss in case of SS failure.

- **Inactive SS Recovery:**
  - If an SS becomes inactive, the system automatically initiates data recovery, retrieving content from a backup SS to ensure data integrity and availability.

- **Automatic Backup Activation:**
  - Mechanisms detect SS failures, triggering the activation of corresponding backup SS to maintain data redundancy and minimize downtime.


### Asynchronous Duplication

Write operations occur asynchronously in the backups without the Naming Server waiting for acknowledgment. Data redundancy is ensured in the background, contributing to fault tolerance without affecting the NM's processing of subsequent requests.

## Bookeeping


### File Handling
The Naming Server maintains a bookkeeping text file opened in append mode to log client requests and acknowledgments.

### Information Recorded
  - Client IP Address
  - Client Port
  - SS Index
  - Request Details
  - Acknowledgment Status (Success/Failure)

### Updating on Requests
  - Every client request triggers an update to the bookkeeping file, recording relevant information.

### Termination Handling
  - When the Naming Server terminates, the bookkeeping file is cleared, ensuring a fresh log for subsequent sessions.



