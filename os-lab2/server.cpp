#include <cstdio>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

volatile sig_atomic_t was_sighup = 0;
volatile sig_atomic_t was_sigint = 0;

void signal_handler(int signal_number)
{
    if (signal_number == SIGHUP)
    {
        was_sighup = 1;
    }
    else
    {
        was_sigint = 1;
    }
}

int main()
{
    int port = 8080;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    bind(server_fd, (sockaddr *)&address, sizeof(address));
    listen(server_fd, 16);

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGHUP, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    sigset_t blocked_mask;
    sigset_t original_mask;
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGHUP);
    sigaddset(&blocked_mask, SIGINT);
    sigprocmask(SIG_BLOCK, &blocked_mask, &original_mask);

    int client_fd = -1;
    char buffer[1024];

    while (!was_sigint)
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;
        if (client_fd != -1)
        {
            FD_SET(client_fd, &read_fds);
            max_fd = client_fd;
        }

        int ready = pselect(max_fd + 1, &read_fds, NULL, NULL, NULL, &original_mask);

        if (was_sighup)
        {
            printf("Received SIGHUP\n");
            was_sighup = 0;
        }

        if (ready > 0 && FD_ISSET(server_fd, &read_fds))
        {
            int new_fd = accept(server_fd, NULL, NULL);
            printf("New connection\n");
            if (client_fd != -1)
            {
                close(new_fd);
            }
            else
            {
                client_fd = new_fd;
            }
        }

        if (ready > 0 && client_fd != -1 && FD_ISSET(client_fd, &read_fds))
        {
            ssize_t received = recv(client_fd, buffer, sizeof(buffer), 0);
            if (received > 0)
            {
                printf("Received %zd bytes\n", received);
            }
            else
            {
                printf("Connection closed\n");
                close(client_fd);
                client_fd = -1;
            }
        }
    }

    printf("Received SIGINT, shutting down\n");
    if (client_fd != -1)
    {
        close(client_fd);
    }
    close(server_fd);
    sigprocmask(SIG_SETMASK, &original_mask, NULL);
    return 0;
}
