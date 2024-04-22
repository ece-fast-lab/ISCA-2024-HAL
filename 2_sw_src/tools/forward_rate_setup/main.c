#include "udp_conf.h"

volatile sig_atomic_t stop;
void inthand(int signum) {
    stop = 1;
}

// Driver code
int main(int argc, char** argv) 
{

    int interval_usec; // usec
    uint32_t feedback_rate;
    int num_cores;
    sscanf(argv[1],"%d",&interval_usec);
    sscanf(argv[2],"%d",&feedback_rate);
    sscanf(argv[3],"%d",&num_cores); // the number of cores

    uint32_t array[2];
    array[0] = 1717921125; // 0x66656564
    array[1] = feedback_rate;

    int sockfd;
    char buffer[MAXLINE];
    struct sockaddr_in servaddr;

    // Ctrl-C handler
    stop = 0;
    signal(SIGINT, inthand);

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    //set timer for recv_socket
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if ( (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv,sizeof(tv))) < 0) {
        perror("socket timeout failed");
        exit(EXIT_FAILURE);
    }

    // Filling server information
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr("192.168.200.52");


    // statistics
    uint64_t tsc_frequency = 200000000;
    uint64_t stat_previous_middle_time, process_middle_time;
    uint64_t total_data, stat_previous_total_data = 0;

    stat_previous_middle_time = tsc_counter();


    while(!stop) {
        process_middle_time = tsc_counter();
        if (process_middle_time - stat_previous_middle_time > tsc_frequency / (USEC_PER_SEC/ interval_usec)) {

            sendto(sockfd, array, sizeof(array),
                0, (const struct sockaddr *) &servaddr,
                sizeof(servaddr));

        }
    }

    return 0;
}
