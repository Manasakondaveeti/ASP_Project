#include <stdio.h>          // Standard input/output operations
#include <stdlib.h>         // Standard library functions
#include <string.h>         // String manipulation functions
#include <unistd.h>         // Symbolic constants and types
#include <arpa/inet.h>      // Definitions for internet operations
#include <sys/socket.h>     // Socket programming functions
#include <dirent.h>         // Directory entry functions
#include <sys/stat.h>       // File information functions
#include <ftw.h>            // File tree walk functions
#include <pwd.h>            // User database functions
#include <time.h>           // Time manipulation functions

#define PORT1 3160          // Port number for server
#define BUFFER_SIZE 1024    // Size of buffer for reading data
#define QUIT_CMD "quitc\n"  // Command to quit the client
#define MAX_FILES 1000      // Maximum number of files in a directory



DIR *d;                            // Directory pointer variable
struct dirent *dir;                // Directory entry pointer variable
char *homeDir = "/home/kondave2/Desktop/Project/Testtar"; // For testing , this directory is being used

	
	/**.......w24ft command..........**/
	
// Function to find files with specified extensions in a directory and its subdirectories
int find_files_with_extensions(char* directory, char* extensions[], int ext_count, char** file_list, int* file_count) {
    DIR* dir;                    // Directory stream pointer
    struct dirent* entry;        // Directory entry pointer
    struct stat info;            // File information structure
    char path[1024];             // Buffer to store file path
    int count = 0;               // Count of files found
    DIR* stack[MAX_FILES];       // Stack to store directory streams
    int top = -1;                // Top index of the stack

    // Open the directory
    if ((dir = opendir(directory)) == NULL) {
        perror("Failed to open directory"); // Print error message if opening directory fails
        return 0; // Return 0 to indicate failure
    }

    stack[++top] = dir; // Push the directory stream onto the stack

    // Loop until the stack is empty
    while (top != -1) {
        dir = stack[top--]; // Pop a directory stream from the stack

        // Loop through all entries in the directory
        while ((entry = readdir(dir)) != NULL) {
            // Skip current and parent directory entries
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            // Construct full path of the entry
            snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

            // Get file information
            if (stat(path, &info) != 0) {
                perror("Failed to get file information"); // Print error message if getting file information fails
                continue; // Continue to next entry
            }

            // Check if the entry is a directory
            if (S_ISDIR(info.st_mode)) {
                // Open the subdirectory
                DIR* subdir = opendir(path);
                if (subdir != NULL) {
                    stack[++top] = subdir; // Push the subdirectory stream onto the stack
                }
            } else {
                printf("File name: %s\n", entry->d_name); // Print file name

                // Check if the file extension matches any of the specified extensions
                int matched = 0; // Flag to indicate if file extension is matched
                char* file_extension = strrchr(entry->d_name, '.'); // Get file extension
                for (int i = 0; i < ext_count; i++) {
                    printf("manasa ext found %s \n", extensions[i]); // Print extension being checked
                    printf("File extension: %s\n", file_extension); // Print file extension
                    // Compare file extension with specified extension
                    if (file_extension != NULL && strcmp(file_extension + 1, extensions[i]) == 0) {
                        matched = 1; // Set matched flag to true
                        break; // Exit loop if extension is matched
                    }
                }
                // If file extension is matched, add file path to the list
                if (matched) {
                    // Check if file count is within maximum limit
                    if (*file_count < MAX_FILES) {
                        file_list[*file_count] = strdup(path); // Duplicate and store file path
                        (*file_count)++; // Increment file count
                        count++; // Increment total count
                    }
                }
            }
        }
        closedir(dir); // Close the directory stream
    }

    return count; // Return total count of files found
}


// Function to execute the 'w24ft' command
void executeW24ftCommand(const char* extensionList, int client) {
    char* token = strtok(extensionList, " "); // Tokenize the extension list
    char* extensions[3] = {NULL, NULL, NULL}; // Array to store up to three file extensions
    int count = 0; // Counter for number of extensions found

    // Parse the extension list
    while (token != NULL && count < 3) {
        extensions[count] = token; // Store the extension in the array
        token = strtok(NULL, " "); // Move to the next token
        count++; // Increment the count
    }
    printf("Ext list count %d \n", count); // Print the count of extensions found

    // Check if the extension list contains up to three different file types
    if (count == 0) {
        write(client, "No file types specified.\n", strlen("No file types specified.\n")); // Send message to client
        return; // Exit the function
    }

    char* file_list[MAX_FILES]; // Array to store file paths
    int file_count = 0; // Variable to store the count of files found
    // Find files with specified extensions in the home directory
    file_count = find_files_with_extensions(homeDir, extensions, count, file_list, &file_count);

    // Check if any files were found
    if (file_count == 0) {
        printf("No file found.\n"); // Print message to indicate no files found
        write(client, "No file found.\n", strlen("No file found.\n")); // Send message to client
    } else {
        // Compress files and send to client
        int result = compress_files_and_send(client, file_list, file_count);
        if (result == 0) {
            // Compression and sending successful
            printf("Files compressed successfully into temp.tar.gz and sent to client.\n");
        } else {
            // Compression or sending failed
            printf("Failed to compress files or send to client.\n");
        }
    }
}


    
 /*.........w24fda 2023-03-31............ */
 // Function to compress files and send the compressed file to the client
int compress_files_and_send(int client, char** file_list, int file_count) {
    // Compress files
    int compression_result = compress_files(file_list, file_count);
    // Check if compression was successful
    if (compression_result != 0) {
        // Compression failed, send error message to client
        char *errorMsg = "Error compressing files.\n";
        write(client, errorMsg, strlen(errorMsg));
        return compression_result; // Return compression error code
    }

    // Open and send temp.tar.gz file to client
    FILE *file = fopen("temp.tar.gz", "rb");
    // Check if file opening was successful
    if (file == NULL) {
        // Failed to open temp.tar.gz, send error message to client
        char *errorMsg = "Error opening compressed file.\n";
        write(client, errorMsg, strlen(errorMsg));
        return -1; // Return error code
    }

    // Read and send file contents
    char buffer[BUFFER_SIZE]; // Buffer to store file data
    size_t bytesRead; // Number of bytes read
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        // Send file contents to client
        if (send(client, buffer, bytesRead, 0) != bytesRead) {
            perror("Failed to send file");
            break; // Break the loop if sending fails
        }
    }

    // Close file
    fclose(file);

    return 0; // Success
}

// Function to parse a date string in the format "YYYY-MM-DD" and convert it to a time_t
time_t parse_date(const char* date_str) {
    struct tm tm = {0}; // Initialize time structure
    strptime(date_str, "%Y-%m-%d", &tm); // Parse date string
    return mktime(&tm); // Convert to time_t and return
}

  // Function to execute the 'w24fda' command
void executeW24fdaCommand(char* cmd, int client) {
    // Determine command type (before or after the date)
    int after = strncmp(cmd, "w24fda", 6) == 0;

    // Extract the date from the command
    char dateStr[BUFFER_SIZE];
    sscanf(cmd + 7, "%s", dateStr); // cmd + 7 to skip past the command and space
    time_t target_date = parse_date(dateStr); // Parse the date string and convert it to a time_t

    char* file_list[MAX_FILES]; // Array to store file paths
    int file_count = 0; // Variable to store the count of files found
    
    // Find files by date
    find_files_by_date(homeDir, target_date, after, file_list, &file_count);

    // Check if any files were found
    if (file_count == 0) {
        char *noFileMsg = "No file found\n"; // Message indicating no files found
        write(client, noFileMsg, strlen(noFileMsg)); // Send message to client
    } else {
        // Compress files and send to client
        int result = compress_files_and_send(client, file_list, file_count);
        if (result == 0) {
            // Compression and sending successful
            printf("Files compressed successfully into temp.tar.gz and sent to client.\n");
        } else {
            // Compression or sending failed
            printf("Failed to compress files or send to client.\n");
        }
    }

    // Cleanup: Free memory allocated for file paths
    for (int i = 0; i < file_count; i++) {
        free(file_list[i]);
    }
}

// Checks if the file's modification time meets the specified criteria
int check_mtime(const char* path, time_t target_date, int after) {
    struct stat info; // Structure to store file information
    // Get file information
    if (stat(path, &info) != 0) {
        perror("Failed to get file information"); // Print error message if getting file information fails
        return 0; // Return false
    }

    // Check if the modification time of the file is after or before the target date
    if (after) {
        return info.st_mtime >= target_date; // Return true if modification time is after target date
    } else {
        return info.st_mtime <= target_date; // Return true if modification time is before target date
    }
}


// Modified version of find_files_in_size_range to include date filtering
void find_files_by_date(char* directory, time_t target_date, int after, char** file_list, int* file_count) {
    DIR* dir; // Directory stream pointer
    struct dirent* entry; // Directory entry pointer
    struct stat info; // File information structure
    char path[1024]; // Buffer to store file path

    // Open the directory
    if ((dir = opendir(directory)) == NULL) {
        perror("Failed to open directory"); // Print error message if opening directory fails
        return; // Exit the function
    }

    // Loop through all entries in the directory
    while ((entry = readdir(dir)) != NULL) {
        // Skip current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Construct full path of the entry
        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        // Check if the file's modification time meets the specified criteria
        if (check_mtime(path, target_date, after)) {
            // Check if the file count is within the maximum limit
            if (*file_count < MAX_FILES) {
                file_list[*file_count] = strdup(path); // Duplicate and store file path
                (*file_count)++; // Increment file count
            }
        } else if (stat(path, &info) == 0 && S_ISDIR(info.st_mode)) {
            // If the entry is a directory, recursively call the function to search within the directory
            find_files_by_date(path, target_date, after, file_list, file_count);
        }
    }

    // Close the directory
    closedir(dir);
}

    
 /*...................   w24fz size1 size2..*/

int compress_files(char* file_list[], int num_files) 
{
    // Initialize teh comand wih opions.
    char command[1000] = "tar -czvf temp.tar.gz";
    // loopnig throuh he lit  fies and add each to the ueds command.
    for (int i = 0; i < num_files; i++) 
    {
         // ensuiring space-separatro
        strcat(command, " ");
        // ensuring crrent filepath cmmand.
        strcat(command, file_list[i]);
    }
    // Execute the tar command
    int status = system(command);
    if (status != 0) {
        fprintf(stderr, "Error: Failed to create tar file\n");
        return status;
    }

    // Tar file created successfully
    return 0;
}
// Function to execute the 'w24fz' command
void executeW24fzCommand(char* cmd, int client) {
    long size1, size2;
    // Extract size parameters from the command
    sscanf(cmd, "%ld %ld", &size1, &size2);

    // Check if size1 and size2 are valid
    if (size1 > size2 || size1 < 0 || size2 < 0) {
        // Send error message to client
        write(client, " size1 must be <= size2 and both must be >= 0\n", strlen(" size1 must be <= size2 and both must be >= 0\n"));
        printf(" size1 must be <= size2 and both must be >= 0\n"); // Print error message
        // Consider sending this error back to the client instead of just printing
        return; // Exit the function
    }

    // Array to store file paths
    char* file_list[MAX_FILES];
    int file_count = 0; // Variable to store the count of files found
    // Find files within the specified size range
    find_files_in_size_range(homeDir, size1, size2, file_list, &file_count);

    // Check if any files were found
    if (file_count == 0) {
        char *noFileMsg = "No file found\n"; // Message indicating no files found
        write(client, noFileMsg, strlen(noFileMsg)); // Send message to client
    } else {
        // Compress files and send to client
        int result = compress_files_and_send(client, file_list, file_count);
        if (result == 0) {
            // Compression and sending successful
            printf("Files compressed successfully into temp.tar.gz and sent to client.\n");
        } else {
            // Compression or sending failed
            printf("Failed to compress files or send to client.\n");
        }
    }

    // Cleanup: Free memory allocated for file paths
    for (int i = 0; i < file_count; i++) {
        free(file_list[i]);
    }
}

 // Function to recursively find files within size range and add to file list
void find_files_in_size_range(char* directory, long size1, long size2, char** file_list, int* file_count) {
    DIR* dir; // Directory stream pointer
    struct dirent* entry; // Directory entry pointer
    struct stat info; // File information structure
    char path[1024]; // Buffer to store file path

    // Open the directory
    if ((dir = opendir(directory)) == NULL) {
        perror("Failed to open directory"); // Print error message if opening directory fails
        return; // Exit the function
    }

    // Loop through all entries in the directory
    while ((entry = readdir(dir)) != NULL) {
        // Skip current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Construct full path of the entry
        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        // Get file information
        if (stat(path, &info) != 0) {
            perror("Failed to get file information"); // Print error message if getting file information fails
            continue; // Skip to the next entry
        }

        // If the entry is a directory, recursively call the function
        if (S_ISDIR(info.st_mode)) {
            find_files_in_size_range(path, size1, size2, file_list, file_count);
        } else if (info.st_size >= size1 && info.st_size <= size2) {
            // Check if the file size is within the specified range
            if (*file_count < MAX_FILES) {
                file_list[*file_count] = strdup(path); // Duplicate and store file path
                (*file_count)++; // Increment file count
            }
        }
    }

    // Close the directory
    closedir(dir);
}


/*....................*/

#define BUFFER_SIZE 1024

/* Function to recursively search for a file in a directory */
void findFile(const char *dirPath, const char *fileName, int clientSock, int *found) {
    if (*found) return; // If file is found, return immediately

    DIR *dir = opendir(dirPath); // Open the directory
    if (dir == NULL) {
        return; // Failed to open directory
    }

    struct dirent *entry;
    // Iterate through directory entries
    while ((entry = readdir(dir)) != NULL && !*found) {
        // Skip current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        // Construct full path of the entry
        char path[BUFFER_SIZE];
        snprintf(path, sizeof(path), "%s/%s", dirPath, entry->d_name);

        struct stat statbuf;
        // Get file status
        if (stat(path, &statbuf) == -1) continue; // Skip if unable to stat file

        // If the entry is a directory, recursively search in the directory
        if (S_ISDIR(statbuf.st_mode)) {
            findFile(path, fileName, clientSock, found);
        } 
        // If the entry is a regular file and its name matches the target file name
        else if (S_ISREG(statbuf.st_mode) && strcmp(entry->d_name, fileName) == 0) {
            char fileInfo[BUFFER_SIZE];
            char modTimeStr[80]; // Buffer for formatted date
            struct tm *tm = localtime(&statbuf.st_mtime); // Convert time_t to tm struct
            strftime(modTimeStr, sizeof(modTimeStr), "%a %d %b %Y %I:%M:%S %p", tm); // Format time

            // Prepare file information string
            snprintf(fileInfo, sizeof(fileInfo), "File: %s, Size: %ld bytes, Modified: %s, Permissions: %o\n",
                     path, statbuf.st_size, modTimeStr, statbuf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
            // Send file information to client
            write(clientSock, fileInfo, strlen(fileInfo));
            *found = 1; // Mark as found
            return;
        }
    }
    closedir(dir); // Close the directory
}

void executeW24fsCommand(const char *fileName, int clientSock) {
    // Execute the 'w24fs' command to search for a file
    int found = 0; // Flag to track if the file is found
    findFile(homeDir, fileName, clientSock, &found); // Search for the file in the directory

    // If the file is not found, send a message to the client
    if (!found) {
        char *msg = "File not found\n"; // Message indicating file not found
        write(clientSock, msg, strlen(msg)); // Send message to client
    }
}

/* Definition of the DirEntry structure */
typedef struct {
    char *name; // Name of the directory entry
    time_t mtime; // Modification time of the directory entry
} DirEntry;

// Enum to define the comparison type
typedef enum {
    SORT_BY_NAME, // Sort by name
    SORT_BY_TIME // Sort by modification time
} SortMode; // Enumeration to define sorting mode

// Global variable to set the sorting mode
SortMode g_sort_mode = SORT_BY_NAME; // Initialize sorting mode to sort by name by default

// Comparison function used by qsort() to compare two elements
int compare(const void *a, const void *b) {
    if (g_sort_mode == SORT_BY_NAME) { // Check the sorting mode
        // Cast pointers to strings and compare alphabetically by name
        const char *nameA = *(const char **)a; // Cast 'a' to a pointer to a pointer to a string
        const char *nameB = *(const char **)b; // Cast 'b' to a pointer to a pointer to a string
        return strcmp(nameA, nameB); // Compare the names alphabetically
    } else { // If sorting by modification time
        // Cast pointers to DirEntry structures and compare by modification time
        time_t timeA = ((const DirEntry *)a)->mtime; // Extract modification time from 'a'
        time_t timeB = ((const DirEntry *)b)->mtime; // Extract modification time from 'b'
        // Compare modification times, with older entries first
        return (timeA > timeB) - (timeA < timeB); // Compare and return result
    }
}
void listDirectoriesonTime(int client) {
    struct stat dirStat; // Struct to store directory information

    printf("%s \n", homeDir); // Print the home directory path
    
    d = opendir(homeDir); // Open the home directory
    if (d) {
        DirEntry *entries = NULL; // Array to store directory entries
        size_t count = 0; // Counter for the number of directory entries

        // Iterate through directory entries
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_DIR) { // Check if the entry is a directory
                if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                    continue; // Skip the current and parent directory entries
                }
                
                char fullPath[PATH_MAX]; // Buffer to store the full path of the directory
                snprintf(fullPath, PATH_MAX, "%s/%s", homeDir, dir->d_name); // Construct the full path
                
                // Get the file information of the directory
                if (stat(fullPath, &dirStat) == 0) {
                    // Allocate memory and save directory info in entries array
                    entries = realloc(entries, sizeof(DirEntry) * (count + 1));
                    entries[count].name = strdup(dir->d_name); // Copy directory name
                    entries[count].mtime = dirStat.st_mtime; // Get modification time
                    count++; // Increment the count
                }
            }
        }

        // Set sorting mode to sort by modification time before calling qsort
        g_sort_mode = SORT_BY_TIME;
        qsort(entries, count, sizeof(DirEntry), compare); // Sort directory entries by modification time
        
        // Check if there are no directories found
        if (count == 0) {
            write(client, "No directories found.\n", strlen("No directories found.\n"));
        } else {
            write(client, "", strlen("")); // Start message with ''
            
            // Send sorted directory names to the client
            for (size_t i = 0; i < count; i++) {
                write(client, entries[i].name, strlen(entries[i].name)); // Send directory name
                write(client, "\n", 1); // Send newline character for formatting
                free(entries[i].name); // Free allocated memory for directory name
            }
        }
        free(entries); // Free the array of DirEntry
        closedir(d); // Close the directory
    } else {
        char *errMsg = "Failed to open directory.\n"; // Error message for failure to open directory
        write(client, errMsg, strlen(errMsg)); // Send error message to the client
    }
}

void listDirectories(int client) {
    printf("%s \n", homeDir); // Print the home directory path
    
    d = opendir(homeDir); // Open the home directory
    if (d) {
        char **dirNames = NULL; // Array to store directory names
        size_t count = 0; // Counter for the number of directory names

        // Iterate through directory entries
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_DIR) { // Check if the entry is a directory
                if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                    continue; // Skip the current and parent directory entries
                }
                // Allocate memory and save directory name
                dirNames = realloc(dirNames, sizeof(char*) * (count + 1));
                dirNames[count] = strdup(dir->d_name); // Copy directory name
                count++; // Increment the count
            }
        }

        // Set sorting mode to sort by name before calling qsort
        g_sort_mode = SORT_BY_NAME;
        qsort(dirNames, count, sizeof(char*), compare); // Sort directory names alphabetically
        
        // Check if there are no directories found
        if (count == 0) {
            write(client, "No directories found.\n", strlen("No directories found.\n"));
        } else {
            write(client, "", strlen("")); // Start message with ''
            
            // Send sorted directory names to the client
            for (size_t i = 0; i < count; i++) {
                write(client, dirNames[i], strlen(dirNames[i])); // Send directory name
                write(client, "\n", 1); // Send newline character for formatting
                free(dirNames[i]); // Free allocated memory for directory name
            }
        }
        free(dirNames); // Free the array of pointers
        closedir(d); // Close the directory
    } else {
        char *errMsg = "Failed to open directory.\n"; // Error message for failure to open directory
        write(client, errMsg, strlen(errMsg)); // Send error message to the client
    }
}
void executeCommand(const char* cmd, int client) {
    // Check if the command is for listing all directories
    if (strncmp(cmd, "dirlist -a", 10) == 0) {
        listDirectories(client); // Execute listDirectories function
    } 
    // Check if the command is for listing directories sorted by time
    else if (strncmp(cmd, "dirlist -t", 10) == 0) {
        listDirectoriesonTime(client); // Execute listDirectoriesonTime function
    }
    // Check if the command is for listing files in a directory
    else if (strncmp(cmd, "w24fs ", 6) == 0) {
        // Extract the filename from the command
        char fileName[BUFFER_SIZE];
        sscanf(cmd + 6, "%s", fileName); 
        executeW24fsCommand(fileName, client); // Execute executeW24fsCommand function
    } 
    // Check if the command is for finding files by size
    else if (strncmp(cmd, "w24fz ", 6) == 0) {
        executeW24fzCommand(cmd + 6, client); // Execute executeW24fzCommand function
    }
    // Check if the command is for finding files by date
    else if (strncmp(cmd, "w24fdb ", 7) == 0 || strncmp(cmd, "w24fda ", 7) == 0) {
        executeW24fdaCommand(cmd, client); // Execute executeW24fdaCommand function
    } 
    // Check if the command is for finding files by type
    else if (strncmp(cmd, "w24ft ", 6) == 0) {
        printf("%s command \n ", cmd); // Print command for debugging
        executeW24ftCommand(cmd + 6, client); // Execute executeW24ftCommand function
    }

    // Add implementations for other commands here
}


/*..............*/
void crequest(int client) {
    char buffer[BUFFER_SIZE]; // Buffer to store received data
    while (1) {
        memset(buffer, 0, BUFFER_SIZE); // Clear buffer before each read operation
        ssize_t bytes_read = read(client, buffer, BUFFER_SIZE - 1); // Read data from client
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0'; // Null-terminate the received data
            if (strncmp(buffer, QUIT_CMD, strlen(QUIT_CMD)) == 0) {
                break; // Exit loop and close connection on "quitc" command
            }
            executeCommand(buffer, client); // Execute the received command
        } else if (bytes_read == 0) {
            break; // Exit loop if client disconnects
        } else {
            perror("read from client failed"); // Print error message on read failure
            break; // Exit loop on read error
        }
    }
    close(client); // Close client connection
}

int main() {
    int server_fd, client_socket; // File descriptors for server and client sockets
    struct sockaddr_in address; // Structure to hold server address information
    int opt = 1; // Option variable
    int addrlen = sizeof(address); // Length of address structure

    // Creating socket file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed"); // Print error message if socket creation fails
        exit(EXIT_FAILURE); // Exit the program with failure status
    }

    // Forcefully attaching socket to the port PORT1
    address.sin_family = AF_INET; // Address family IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP address
    address.sin_port = htons(PORT1); // Set port number

    // Binding socket to the address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed"); // Print error message if binding fails
        exit(EXIT_FAILURE); // Exit the program with failure status
    }

    // Listening for incoming connections
    if (listen(server_fd, 10) < 0) {
        perror("listen"); // Print error message if listening fails
        exit(EXIT_FAILURE); // Exit the program with failure status
    }

    // Accepting incoming connections in a loop
    while (1) {
        printf("Waiting for connections...\n");
        // Accepting a client connection
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            perror("accept"); // Print error message if accepting connection fails
            continue; // Continue accepting other connections even if one fails
        }

        // Forking a child process to handle the client
        if (!fork()) { // Child process
            close(server_fd); // Child doesn't need the listener
            crequest(client_socket); // Handling client request
            exit(0); // Exit child process
        }
        close(client_socket); // Parent doesn't need this specific client socket
    }
    // In a real application, we'd also need to handle closing the server_fd properly.
    return 0; // Exit main function with success status
}

