#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void search_file(const char *base_path, const char *target_file, char *results) {
    DIR *dir = opendir(base_path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    char path[BUFFER_SIZE];

    while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;

    snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name);
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) continue;

    // 🔍 Debug print
    printf("Checking: %s\n", path);

    if (S_ISDIR(statbuf.st_mode)) {
        search_file(path, target_file, results);
    } else if (S_ISREG(statbuf.st_mode)) {
        if (strcmp(entry->d_name, target_file) == 0) {
            printf("Match found in: %s\n", base_path); // 🔍 Show where match was found
            strncat(results, base_path, BUFFER_SIZE * 10 - strlen(results) - 2);
            strncat(results, "\n", BUFFER_SIZE * 10 - strlen(results) - 1);
        }
    }
}

    closedir(dir);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    char results[BUFFER_SIZE * 10] = {0}; // Store matching directories

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Setup address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept client
    client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (client_fd < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Read file name
    read(client_fd, buffer, sizeof(buffer));
    buffer[strcspn(buffer, "\n")] = 0; // Strip newline
    printf("Searching for file: '%s'\n", buffer);

    // Search
    search_file("./testdir", buffer, results);

    // Send results
    send(client_fd, results, strlen(results), 0);

    close(client_fd);
    close(server_fd);
    return 0;
}
