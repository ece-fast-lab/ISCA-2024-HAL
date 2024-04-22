#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <signal.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_timer.h>
#include <rte_random.h>
#include <rte_malloc.h>
#include <math.h>

#include "../traffic_receiver/test_app/bm25_app.h"
#include "../traffic_receiver/test_app/bayes_app.h"
#include "../traffic_receiver/test_app/knn_app.h"
#include "../traffic_receiver/test_app/nat_app.h"


// Configurable number of RX/TX ring descriptors
#define RX_DESC_DEFAULT 1024
#define TX_DESC_DEFAULT 1024

#define NUM_MBUFS 4095
#define MBUF_CACHE_SIZE 512
#define BURST_SIZE 64

#define MAX_RX_CORES 16

#define RX_WORKER_RING_SIZE 256

static const struct rte_eth_conf port_conf_default = {
    .rxmode = {
        .mq_mode = ETH_MQ_RX_RSS,
	    .max_rx_pkt_len = RTE_ETHER_MAX_LEN,
    },
    .rx_adv_conf = {
        .rss_conf = {
            .rss_key = NULL,
            .rss_hf = ETH_RSS_IP | ETH_RSS_UDP | ETH_RSS_TCP,
        }
    }
};

struct my_pps { 
    double rx_data[600]; 
    double tx_data[600]; 
    double work_data[600];
};

static uint16_t test_pps = 1;
volatile struct my_pps pps_arr[MAX_RX_CORES];
static int pps_size = 0;

static struct rte_ether_addr my_ether_addr;
static uint64_t smartnic_cap = 3000000;
static uint32_t is_forwarding = 1;
static uint16_t rx_port_id = 2;
static uint16_t tx_port_id = 0;
static unsigned int interval = 1;
static uint32_t pkts_per_sec = 0;
static struct rte_ether_addr src_mac;
static struct rte_ether_addr dst_mac;
static rte_be32_t src_ip;
static rte_be32_t dst_ip;

// used for applications
static int app_id;
static int app_arg1;
static int app_arg2;

// used for spliter
static int split_num_core;
static int nb_worker_cores;

static uint64_t last_lcore_rx_pkts[MAX_RX_CORES];
static volatile uint64_t lcore_rx_pkts[MAX_RX_CORES];
static volatile uint64_t lcore_rx_pkts_split[MAX_RX_CORES];

static uint64_t last_lcore_tx_pkts[MAX_RX_CORES];
static volatile uint64_t lcore_tx_pkts[MAX_RX_CORES];

static uint64_t last_lcore_work_pkts[MAX_RX_CORES];
static volatile uint64_t lcore_work_pkts[MAX_RX_CORES];

struct app_lcore_params {
    struct rte_ring *rx_worker_ring[MAX_RX_CORES];
    struct rte_ring *worker_rx_ring[MAX_RX_CORES];
    uint32_t n_rings;
} __rte_cache_aligned;

struct app_lcore_params lcore_params_array[MAX_RX_CORES];

static int ring_init(uint16_t nb_total_cores, uint16_t nb_forward_cores);

// Initialzie a port with the number of tx/rx rings and the ring size
static int port_init(uint16_t port, struct rte_mempool *mbuf_pool,
                    uint16_t nb_tx_rings, uint16_t nb_rx_rings,
                    uint16_t tx_desc_size, uint16_t rx_desc_size);

// RX packet processing main loop function
static int lcore_rx_worker();
static int lcore_app_worker();

// Update mac address when forwarding
static void l2fwd_mac_updating(struct rte_mbuf *bufs);

// Keep receiving packet until set to 0
static volatile int keep_receiving = 1;
static void stop_rx(int sig) { keep_receiving = 0; }

// Print functions
void print_mac(uint16_t port, struct rte_ether_addr mac_addr) {
    printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
			" %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
			port, mac_addr.addr_bytes[0], mac_addr.addr_bytes[1], mac_addr.addr_bytes[2],
           mac_addr.addr_bytes[3], mac_addr.addr_bytes[4], mac_addr.addr_bytes[5]);
}

void print_ip(rte_be32_t ip) {
    struct in_addr addr = {.s_addr = ip};
    printf("%s\n", inet_ntoa(addr));
}

static void print_config()
{
    printf("================ Configuration ================\n");
    printf("Port:           %u\n", rx_port_id);
    printf("Interval:       %u sec\n", interval);
    printf("Packet Rate:    ");
    if (pkts_per_sec == 0) {
        printf("N/A (no rate limiting)\n");
    } else {
        printf("%u\n", pkts_per_sec);
    }

    if (app_id == 0) {
        printf("No application is running\n");
    } else {
        printf("Running application %d\n", app_id);
    }

    if (is_forwarding == 0) {
        printf("Software forwarding is not enabled\n");
    } else {
        printf("Running software forwarding with %d cores\n", rte_lcore_count() - split_num_core);
    }
    printf("===============================================\n");
}

static int ring_init(uint16_t nb_total_cores, uint16_t nb_forward_cores)
{
    unsigned i = 0;
    unsigned j = 0;

    // set n_rings to zero
    for (i = 0; i < nb_total_cores; i++) {
        lcore_params_array[i].n_rings = 0;
    }

    if (nb_worker_cores == 0) {
        printf("nb_worker_cores is zero\n");
        return 0;
    }

    if (nb_forward_cores == 0) {
        printf("nb_forward_cores is zero\n");
        return 0;
    }

    // set rx_worker_ring
    for (i = 0; i < nb_forward_cores; i++) {
        unsigned socket_io = rte_lcore_to_socket_id(i);
        for (j = 0; j < nb_worker_cores; j++) {
            char name[32];
            struct rte_ring *ring = NULL;

            snprintf(name, sizeof(name), "app_ring_rx_s%u_io%u_w%u", socket_io, i, j);
            ring = rte_ring_create(name, RX_WORKER_RING_SIZE, socket_io, RING_F_SP_ENQ | RING_F_SC_DEQ);

            lcore_params_array[i].rx_worker_ring[j] = ring;
            lcore_params_array[i].n_rings++;

            lcore_params_array[nb_forward_cores+j].rx_worker_ring[i] = ring;
            lcore_params_array[nb_forward_cores+j].n_rings++;
        }
    }

    // check n_rings equal to nb_worker_cores / nb_forward_cores
    for (i = 0; i < nb_total_cores; i++) {
        if (i < nb_forward_cores) {
            if (lcore_params_array[i].n_rings != nb_worker_cores) {
                printf("lcore %u has %u rings\n", i, lcore_params_array[i].n_rings);
                return -1;
            }
        } else {
            if (lcore_params_array[i].n_rings != nb_forward_cores) {
                printf("lcore %u has %u rings\n", i, lcore_params_array[i].n_rings);
                return -1;
            }
        }
    }

    return 0;
}


// Initialzie a port with the number of tx/rx rings and the ring size
static int port_init(uint16_t port, struct rte_mempool *mbuf_pool,
                    uint16_t nb_tx_rings, uint16_t nb_rx_rings,
                    uint16_t tx_desc_size, uint16_t rx_desc_size)
{                                                
    int retval;
    struct rte_eth_conf local_port_conf = port_conf_default;
    uint16_t nb_txd = tx_desc_size;
    uint16_t nb_rxd = rx_desc_size;
    struct rte_eth_dev_info dev_info;

    printf("Initalizing port %hu ...\n", port);

    if (!rte_eth_dev_is_valid_port(port)) {
        return -1;
    }

    int socket_id = rte_eth_dev_socket_id(port);

    // Get device information
    retval = rte_eth_dev_info_get(port, &dev_info);
    if (retval != 0) {
        fprintf(stderr, "rte_eth_dev_info_get failed  to get port %u info: %s\n", port, strerror(-retval));
        return retval;
    }

    // if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE) {
	// 	local_port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;
    // }

    // Configure RSS
    local_port_conf.rx_adv_conf.rss_conf.rss_hf &= dev_info.flow_type_rss_offloads;
    if (local_port_conf.rx_adv_conf.rss_conf.rss_hf != port_conf_default.rx_adv_conf.rss_conf.rss_hf) {
        printf("Port %u modified RSS hash function "
                "based on hardware support,"
                "requested:%#"PRIx64" configured:%#"PRIx64"\n",
                port, 
                port_conf_default.rx_adv_conf.rss_conf.rss_hf,
                local_port_conf.rx_adv_conf.rss_conf.rss_hf);
    }

    // Configure the Ethernet device
    retval = rte_eth_dev_configure(port, nb_rx_rings, nb_tx_rings, &local_port_conf);
    if (retval != 0) {
        fprintf(stderr, "rte_eth_dev_configure\n");
        return retval;
    }

    // Adjust number of descriptors
    retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
    if (retval != 0) {
        fprintf(stderr, "rte_eth_dev_adjust_nb_rx_tx_desc failed\n");
        return retval;
    }

    // Allocate and set up nb_tx_rings TX queue
    for (uint16_t q = 0; q < nb_tx_rings; q++) {
        retval = rte_eth_tx_queue_setup(port, q, nb_txd, socket_id, NULL);
        if (retval < 0) {
            fprintf(stderr, "rte_eth_tx_queue_setup failed for queue %hu\n", q);
            return retval;
        }
    }
    printf("Set up %hu TX rings (%hu descriptors per ring)\n", nb_tx_rings, nb_txd);

    // Allocate and set up nb_rx_rings RX queue
    for (uint16_t q = 0; q < nb_rx_rings; q++) {
        retval = rte_eth_rx_queue_setup(port, q, nb_rxd, socket_id, NULL, mbuf_pool);
        if (retval < 0) {
            fprintf(stderr, "rte_eth_rx_queue_setup failed for queue %hu\n", q);
            return retval;
        }
    }
    printf("Set up %hu RX rings (%hu descriptors per ring)\n", nb_rx_rings, nb_rxd);

    // Start the Ethernet port.
    retval = rte_eth_dev_start(port);
    if (retval < 0) {
        fprintf(stderr, "rte_eth_dev_start failed\n");
        return retval;
    }

    // Display the port MAC address
    struct rte_ether_addr addr;
    retval = rte_eth_macaddr_get(port, &addr);
    if (retval != 0) {
        fprintf(stderr, "rte_eth_macaddr_get failed\n");
        return retval;
    }
    print_mac(port, addr);

    my_ether_addr = addr;

#if 0
    // Enable RX in promiscuous mode for the Ethernet device.
    retval = rte_eth_promiscuous_enable(port);
    // End of setting RX port in promiscuous mode.
    if (retval != 0) {
        fprintf(stderr, "rte_eth_promiscuous_enable failed\n");
        return retval;
    }
#endif

    return 0;
}


// Update destination MAC
static void l2fwd_mac_updating(struct rte_mbuf *bufs)
{
    struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(bufs, struct rte_ether_hdr*);
    // update destination MAC
    rte_ether_addr_copy(&dst_mac, &eth_hdr->d_addr);
    return;
}


// Parse arguments
static int parse_rx_port(char *arg)
{
    long n;
    char **endptr;
    n = (uint16_t)strtol(arg, endptr, 10);
    if (n < 0 || n >= rte_eth_dev_count_avail()) {
        fprintf(stderr, "Invalid port\n");
        return -1;
    }
    rx_port_id = (uint16_t)n;
    return 0;
}

static int parse_tx_port(char *arg)
{
    long n;
    char **endptr;
    n = (uint16_t)strtol(arg, endptr, 10);
    if (n < 0 || n >= rte_eth_dev_count_avail()) {
        fprintf(stderr, "Invalid port\n");
        return -1;
    }
    tx_port_id = (uint16_t)n;
    return 0;
}

static int parse_interval(char *arg) {
    unsigned int n;
    char **endptr;
    n = (uint16_t)strtoul(arg, endptr, 10);
    if (n == 0) {
        fprintf(stderr, "Invalid interval, should be positve\n");
        return -1;
    }
    interval = n;
    return 0;
}


static int parse_forward(char *arg)
{
    unsigned int n;
    char **endptr;
    n = (uint32_t)strtoul(optarg, endptr, 10);
    if (n != 0 && n != 1) {
        fprintf(stderr, "FORWARD should be 0 (split) or 1 (forward)\n");
        return -1;
    }
    is_forwarding = n;
    return 0;
}

static int parse_pkt_rate(char *arg)
{
    uint32_t n;
    char **endptr;
    n = (uint32_t)strtoul(arg, endptr, 10);
    if (n == 0) {
        fprintf(stderr, "PKT_RATE should be a positive integer argument\n");
        return -1;
    }
    pkts_per_sec = n;
    return 0;
}

static int parse_smartnic_cap(char *arg)
{
    uint32_t n;
    char **endptr;
    n = (uint32_t)strtoul(arg, endptr, 10);
    if (n == 0) {
        fprintf(stderr, "SMARTNIC_CAP should be a positive integer argument\n");
        return -1;
    }
    smartnic_cap = n;
    return 0;
}

static int parse_src_mac(char *arg)
{
    if (rte_ether_unformat_addr(arg, &src_mac) < 0) {
        fprintf(stderr, "Invalid SRC_MAC %s\n", arg);
        return -1;
    }

    return 0;
}

static int parse_dst_mac(char *arg)
{
    if (rte_ether_unformat_addr(arg, &dst_mac) < 0) {
        fprintf(stderr, "Invalid DST_MAC %s\n", arg);
        return -1;
    }

    return 0;
}

static int parse_src_ip(char *arg)
{
    if (inet_pton(AF_INET, arg, &src_ip) <= 0) {
        fprintf(stderr, "Invalid SRC_IP %s\n", arg);
        return -1;
    }
    return 0;
}

static int parse_dst_ip(char *arg)
{
    if (inet_pton(AF_INET, arg, &dst_ip) <= 0) {
        fprintf(stderr, "Invalid DST_IP %s\n", arg);
        return -1;
    }
    return 0;
}

static int parse_app_id(char *arg)
{
    uint32_t n;
    char **endptr;
    n = (uint32_t)strtoul(arg, endptr, 10);
    if (n == 0) {
        fprintf(stderr, "PKT_RATE should be a positive integer argument\n");
        return -1;
    }
    app_id = n;
    return 0;
}

static int parse_app_arg1(char *arg)
{
    uint32_t n;
    char **endptr;
    n = (uint32_t)strtoul(arg, endptr, 10);
    if (n == 0) {
        fprintf(stderr, "PKT_RATE should be a positive integer argument\n");
        return -1;
    }
    app_arg1 = n;
    return 0;
}

static int parse_app_arg2(char *arg)
{
    uint32_t n;
    char **endptr;
    n = (uint32_t)strtoul(arg, endptr, 10);
    if (n == 0) {
        fprintf(stderr, "PKT_RATE should be a positive integer argument\n");
        return -1;
    }
    app_arg2 = n;
    return 0;
}

static void print_usage(const char *prgname)
{
    printf("%s [EAL options] -- [-p PORT] [-i INTERVAL] [-r RATE]\n"
           "    -p, --rx-port           rx port to receive packets (default %hu)\n"
           "    -q, --tx-port           tx port to receive packets (default %hu)\n"
           "    -i, --interval          stats retrival period in second (default %u)\n"
           "    -r, --packet-rate       maximum packet sending rate in packet per second (no rate limiting by default)\n"
           "    -B, --source-mac        source MAC address\n"
           "    -E, --dest-mac          destination MAC address\n"
           "    -j, --source-ip         source IP address\n"
           "    -J, --dest-ip           destination IP address\n"
           "    -f, --forward           pure forwarding (1) or spliter (0) (default %u)\n"
           "    -c, --smartnic-cap      maximum capability of SmartNIC ARM cores (pps)\n"
           "    -a, --app-id            application id\n"
           "    -b, --app-arg1          first application argument\n"
           "    -d, --app-arg2          second application argument\n"
           "    -s, --split_num_core    number of cores used for receiving packets \n"
           "    -h, --help              print usage of the program\n",
           prgname, rx_port_id, tx_port_id, interval, is_forwarding);
}

static struct option long_options[] = {
    {"rx-port",     required_argument,  0,  'p'},
    {"tx-port",     required_argument,  0,  'q'},
    {"interval",    required_argument,  0,  'i'},
    {"rate",        required_argument,  0,  'r'},
    {"source-mac",  required_argument,  0,  'B'},
    {"dest-mac",    required_argument,  0,  'E'},
    {"source-ip",   required_argument,  0,  'j'},
    {"dest-ip",     required_argument,  0,  'J'},
    {"forward",     required_argument,  0,  'f'},
    {"cap",         required_argument,  0,  'c'},
    {"app-id",      required_argument,  0,  'a'},
    {"app-arg1",    required_argument,  0,  'b'},
    {"app-arg2",    required_argument,  0,  'd'},
    {"split-core",  required_argument,  0,  's'},
    {"help",        no_argument,        0,  'h'},
    {0,             0,                  0,  0 }
};

static int parse_split_num_core(char *arg)
{
    uint32_t n;
    char **endptr;
    n = (uint32_t)strtoul(arg, endptr, 10);
    if (n == 0) {
        fprintf(stderr, "PKT_RATE should be a positive integer argument\n");
        return -1;
    }
    split_num_core = n;
    return 0;
}

static int parse_args(int argc, char **argv)
{
    char *prgname = argv[0];
    const char short_options[] = "p:q:i:r:B:E:j:J:f:c:a:b:d:s:h";
    int c;
    int ret;
    int nb_required_args = 0;

    while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != EOF) {
        switch (c) {
            case 'p':
                ret = parse_rx_port(optarg);
                if (ret < 0) {
                    printf("Failed to parse port\n");
                    return -1;
                }
                break;

            case 'q':
                ret = parse_tx_port(optarg);
                if (ret < 0) {
                    printf("Failed to parse port\n");
                    return -1;
                }
                break;

            case 'i':
                ret = parse_interval(optarg);
                if (ret < 0) {
                    printf("Failed to parse interval\n");
                    return -1;
                }
                break;

            case 'f':
                ret = parse_forward(optarg);
                if (ret < 0) {
                    printf("Failed to parse forward\n");
                    return -1;
                }
                break;

            case 'r':
                ret = parse_pkt_rate(optarg);
                if (ret < 0) {
                    printf("Failed to parse pkt rate\n");
                    return -1;
                }
                break;

            case 'c':
                ret = parse_smartnic_cap(optarg);
                if (ret < 0) {
                    printf("Failed to parse smartnic capability\n");
                    return -1;
                }
                break;

            case 'B':
                ret = parse_src_mac(optarg);
                if (ret < 0) {
                    return -1;
                }
                break;

            case 'E':
                ret = parse_dst_mac(optarg);
                if (ret < 0) {
                    return -1;
                }
                nb_required_args++;
                break;

            case 'j':
                ret = parse_src_ip(optarg);
                if (ret < 0) {
                    return -1;
                }
                break;

            case 'J':
                ret = parse_dst_ip(optarg);
                if (ret < 0) {
                    return -1;
                }
                break;

            case 'a':
                ret = parse_app_id(optarg);
                if (ret < 0) {
                    printf("Failed to parse app id\n");
                    return -1;
                }
                break;
            
            case 'b':
                ret = parse_app_arg1(optarg);
                if (ret < 0) {
                    printf("Failed to parse app arg 1\n");
                    return -1;
                }
                break;

            case 'd':
                ret = parse_app_arg2(optarg);
                if (ret < 0) {
                    printf("Failed to parse app arg 2\n");
                    return -1;
                }
                break;
            
            case 's':
                ret = parse_split_num_core(optarg);
                if (ret < 0) {
                    printf("Failed to parse num split core\n");
                    return -1;
                }
                break;

            case 'h':
            default:
                print_usage(prgname);
                return -1;
        }
    }

    if (nb_required_args != 1) {
        fprintf(stderr, "<dest_mac> is required\n");
        print_usage(prgname);
        return -1;
    }

    if (optind >= 0) {
        argv[optind-1] = prgname;
    }
    optind = 1;

	return 0;
}

#endif /* _MAIN_H_ */