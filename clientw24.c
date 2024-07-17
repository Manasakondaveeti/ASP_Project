#include <stdio.h>      // Standard input/output operations
#include <stdlib.h>     // Standard library functions
#include <string.h>     // String manipulation functions
#include <unistd.h>     // Symbolic constants and types
#include <arpa/inet.h>  // Definitions for internet operations
#include <sys/socket.h> // Socket programming functions
#include <signal.h>     // Signal handling functions

#define PORT1 3160      // Port for serverw24
#define PORT2 3252      // Port for mirror1
#define PORT3 3354      // Port for mirror2

#define BUFFER_SIZE 1024 // Define the constant BUFFER_SIZE with a value of 1024

// Function to read connection count from a file
int readConnectionCount() {
    FILE *file = fopen("connection_count.txt", "r+"); // Open file for reading and writing
    int count; // Variable to store connection count
    
    // Check if file opening failed
    if (file == NULL) {
        // Try opening the file for writing
        file = fopen("connection_count.txt", "w+");
        // Check if opening for writing also failed
        if (file == NULL) {
            perror("Failed to open file"); // Print error message
            exit(EXIT_FAILURE); // Exit the program with failure status
        }
        count = 1; // Initialize count to 1 if file didn't exist before
    } else {
        // File opened successfully, try reading count from the file
        if (fscanf(file, "%d", &count) != 1) {
            count = 1; // Default to 1 if reading fails
        }
    }
    fclose(file); // Close the file
    return count; // Return the read or default count
}

// Function to increment the connection count and update it in the file
void incrementConnectionCount(int count) {
    FILE *file = fopen("connection_count.txt", "w"); // Open file for writing (truncate existing content)
    
    // Check if file opening failed
    if (file == NULL) {
        perror("Failed to open file"); // Print error message
        exit(EXIT_FAILURE); // Exit the program with failure status
    }
    
    fprintf(file, "%d", count + 1); // Write incremented count to the file
    fclose(file); // Close the file
}

// String constant representing the exit message
const char *exitMsg = "quitc\n";

// Define a file name for a tar.gz file to download
#define FILE_NAME "w24project/downloaded.tar.gz"




// Function to handle client requests
void crequest(int sock) {
    int n; // Variable to store read/write operation results
    pid_t pid; // Process ID variable
    char message[BUFFER_SIZE]; // Buffer to store messages

    // Fork the process
    pid = fork();

    // Check if fork failed
    if (pid < 0) {
        perror("Fork failed"); // Print error message
        exit(EXIT_FAILURE); // Exit the program with failure status
    }

    // Parent process
    if (pid > 0) {
	
	// Variables for reading from socket and writing to file
	char buffer[BUFFER_SIZE];
	ssize_t bytesRead;
	 // Open file for writing in binary mode
    
	
	
	// Loop for reading from socket and writing to file
	while (1) {
            n = read(sock, message, BUFFER_SIZE - 1); // Read from socket
            
            // Check for read error
            if (n < 0) {
                perror("read failed"); // Print error message
                break; // Exit loop on failure
            } else if (n == 0) {
                printf("Connection closed\n"); // Print message if connection is closed
                break; // Exit loop if connection is closed
            }
            
            // Check for end of file marker
            else if (n > 150) {
				FILE *file = fopen(FILE_NAME, "wb");
				// Check if file opening failed
				if (file == NULL) {
					perror("Failed to create file"); // Print error message
					return; // Return from function
				}
	
                if (strncmp(message, "EOF", 3) == 0) {
                    break; // Exit loop if end of file marker is found
                }
                
                // Write message to file
                size_t bytesWritten = fwrite(message, 1, n, file);
                
               
                
                printf("Bytes read: %zd, Bytes written: %zd\n", n, bytesWritten); // Print bytes read and written
								
									// Flush and close the file
					int flushResult = fflush(file);
					if (flushResult != 0) {
						perror("Error flushing file"); // Print error message
					}

					int closeResult = fclose(file);
					if (closeResult != 0) {
						perror("Error closing file"); // Print error message
					}
					
					// Print file save status
					if (flushResult == 0 && closeResult == 0) {
						printf("File saved successfully\n");
						
					} else {
						printf("File not saved\n");
					}
					
					// Check for error while reading from socket
					if (bytesRead < 0) {
						perror("Error while reading from socket"); // Print error message
					}
										
					
					
            }
			else {
            message[n] = '\0'; // Null terminate message

            // Tokenize and print message
            char *line = strtok(message, "\n");
            while (line != NULL) {
                fprintf(stderr, "%s\n", line); // Print each line
                line = strtok(NULL, "\n"); // Move to next line
			}
            }
        }
		
	
       
    } else { // Child process
	
	// Loop for reading commands from user input
	while (1) {
	sleep(2);
	
            printf("clientw24$ "); // Print command prompt
            fflush(stdout); // Flush standard output
            
            n = read(STDIN_FILENO, message, BUFFER_SIZE - 1); // Read from standard input
            
            // Check for read error or end-of-file
            if (n <= 0) {
                perror("read failed or EOF"); // Print error message
                break; // Exit loop on failure or end-of-file
            }
            message[n] = '\0'; // Null terminate message

            // Basic syntax verification for different commands
            if (strncmp(message, "dirlist ", 8) == 0) {
                if (strcmp(message, "dirlist -a\n") != 0 && strcmp(message, "dirlist -t\n") != 0) {
                    printf("Error: dirlist expects '-a' or '-t' as an argument\n"); // Print error message
                    continue; // Skip further processing and ask for input again
                }
                
            } else if (strncmp(message, "w24fs ", 6) == 0) {
                // Further checks can be added to verify the file name
            } else if (strncmp(message, "w24fz ", 6) == 0) {
                int start, end;
                if (sscanf(message, "w24fz %d %d\n", &start, &end) != 2) {
                    printf("Error: w24fz expects two numeric arguments\n"); // Print error message
                    continue; // Skip further processing and ask for input again
                }
            } else if (strncmp(message, "w24ft ", 6) == 0) {
		// Count the number of file types
                char* message_copy = strdup(message);
                char* args = message_copy + 6;
                char* token = strtok(args, " ");
                int count = 0;
                while (token != NULL) {
                    count++;
                    token = strtok(NULL, " ");
                }
                if (count < 1 || count > 3) {
                    printf("Invalid number of file types. Must be between 1 and 3.\n"); // Print error message
                    continue; // Skip further processing and ask for input again
                }
                free(message_copy); // Free memory allocated for message copy
            } else if (strncmp(message, "w24fdb ", 7) == 0 || strncmp(message, "w24fda ", 7) == 0) {
                int y, m, d;
                if (sscanf(message, "w24fd%*c %d-%d-%d\n", &y, &m, &d) != 3) {
                    printf("Error: w24fd expects a date in 'YYYY-MM-DD' format\n"); // Print error message
                    continue; // Skip further processing and ask for input again
                }
            } else if (strcasecmp(message, exitMsg) == 0) {
                printf("Bye Bye!!\n"); // Print exit message
                close(sock); // Close socket
                kill(getppid(), SIGTERM);  // Kill parent process
                break; // Exit loop
            } else {
                printf("Error: Unknown command or incorrect syntax\n"); // Print error message
                continue; // Skip further processing and ask for input again
            }

            // If syntax is correct, send the command to the server
            write(sock, message, n); // Write to socket
        }
	_exit(0); // Exit child process
    }
}
// Main function to connect to the server and handle client requests
int main(int argc, char const *argv[]) {
    // Check if the correct number of command-line arguments is provided
    if (argc != 2) {
        printf("Usage: %s <Server IP>\n", argv[0]); // Print usage message
        return -1; // Return with error status
    }

    // Read connection count from file
    int count = readConnectionCount();
    // Increment connection count
    incrementConnectionCount(count);

    int port;
    // Determine the server port to connect based on the count
    if (count <= 3) {
        port = PORT1;
    } else if (count <= 6) {
        port = PORT2;
    } else if (count <= 9) {
        port = PORT3;
    } else {
        int mod = (count - 10) % 3;
        switch (mod) {
            case 0: port = PORT1; break;
            case 1: port = PORT2; break;
            case 2: port = PORT3; break;
        }
    }

    int sock = 0; // Socket file descriptor
    struct sockaddr_in serv_addr; // Server address structure

    // Create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n"); // Print error message
        return -1; // Return with error status
    }

    // Set server address parameters
    serv_addr.sin_family = AF_INET; // IPv4
    serv_addr.sin_port = htons(port); // Convert port number to network byte order

    // Convert IPv4 address from text to binary form and set it in server address structure
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n"); // Print error message
        return -1; // Return with error status
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n"); // Print error message
        return -1; // Return with error status
    }

    printf("Connected to server on port %d\n", port); // Print success message

    // Function call to handle client requests
    crequest(sock);

    // Close the socket
    close(sock);

    return 0; // Return with success status
}




