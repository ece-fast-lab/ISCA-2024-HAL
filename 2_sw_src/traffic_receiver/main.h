#ifndef _MAIN_H_
#define _MAIN_H_


#include <stdio.h>
#include <stdlib.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_timer.h>
#include <rte_mbuf.h>
#include <getopt.h>

#include "test_app/bm25_app.h"
#include "test_app/bayes_app.h"
#include "test_app/knn_app.h"
#include "test_app/nat_app.h"

#include "test_app/kvs_app.h"
#include "test_app/count_app.h"
#include "test_app/ema_app.h"

#include "test_app/crypto_app.h"

// Algorithm
#define HINT_MORE_DELTA 1000
#define HINT_MORE_STEP_1 100
#define HINT_MORE_STEP_2 4
#define HINT_MORE_STEP_3 2
#define HINT_MORE_STEP_4 1
#define HINT_MORE_WM_1 0
#define HINT_MORE_WM_2 0.25
#define HINT_MORE_WM_3 0.5
#define HINT_MORE_WM_4 1

#define HINT_LESS_DELTA 10000
#define HINT_LESS_STEP_1 500
#define HINT_LESS_STEP_2 100
#define HINT_LESS_STEP_3 10
#define HINT_LESS_WM_1 8
#define HINT_LESS_WM_2 4
#define HINT_LESS_WM_3 2

#define FEEDBACK_MORE_WM 1000
#define FEEDBACK_LESS_WM 1000
#define FEEDBACK_STEP 1000




#define CORE_MORE_WM_1 1.5
#define CORE_MORE_WM_2 1
#define CORE_MORE_STEP_1 25
#define CORE_MORE_STEP_2 1

#define CORE_LESS_WM_1 0.25
#define CORE_LESS_STEP_1 1

#define CORE_CHANGE_WM 1000


// Configurable number of RX/TX ring descriptors
#define RX_DESC_DEFAULT 1024
#define TX_DESC_DEFAULT 1024

#define NUM_MBUFS 4095
#define MBUF_CACHE_SIZE 512
#define BURST_SIZE 128

#define MAX_RX_CORES 32
#define NUM_RX_QUEUES 8

#define MAX_TIME 1000

static uint16_t pkt_size = 1476;

static struct rte_ether_addr src_mac;
static struct rte_ether_addr dst_mac;
static rte_be32_t src_ip;
static rte_be32_t dst_ip;

struct app_lcore_params {
    volatile uint16_t n_rx_queues;
    volatile uint16_t rx_queues_start_index;
    volatile uint16_t working;
} __rte_cache_aligned;


struct app_lcore_params lcore_params_array[MAX_RX_CORES];

static const struct rte_eth_conf port_conf_default = {
    .rxmode = {
        // .mq_mode = RTE_ETH_MQ_RX_RSS,
        .mq_mode = ETH_MQ_RX_RSS,
	    .max_lro_pkt_size = RTE_ETHER_MAX_LEN,
    },
    .rx_adv_conf = {
        .rss_conf = {
            .rss_key = NULL,
            // .rss_hf = RTE_ETH_RSS_IP | RTE_ETH_RSS_UDP | RTE_ETH_RSS_TCP,
            .rss_hf = ETH_RSS_IP | ETH_RSS_UDP | ETH_RSS_TCP,
        }
    },
    .txmode = {
        // .mq_mode = RTE_ETH_MQ_TX_NONE
        .mq_mode = ETH_MQ_TX_NONE
    }
};

struct my_pps { 
    double data[MAX_TIME]; 
}; 
static uint16_t test_pps = 1;
volatile struct my_pps pps_arr[MAX_RX_CORES];
static int pps_size = 0;


static struct rte_ether_addr my_ether_addr;
static uint16_t port_id = 0;
static unsigned int interval = 1;
static uint32_t latency_size = 10000;

// used for applications
static int app_id;
static int app_arg1;
static int app_arg2;

static int send_feedback = 0;

static uint64_t last_lcore_rx_pkts[MAX_RX_CORES];
static volatile uint64_t lcore_rx_pkts[MAX_RX_CORES];
static uint64_t last_lcore_tx_pkts[MAX_RX_CORES];
static volatile uint64_t lcore_tx_pkts[MAX_RX_CORES];

static volatile uint64_t queue_rx_pkts[NUM_RX_QUEUES];
static volatile uint64_t zero_rx_lcore_pkt_count[MAX_RX_CORES];

static volatile int dynamic_num_cores = 0;

// Generate packet
static void generate_packet(struct rte_mbuf *pkt,
                           uint16_t pkt_len,
                           struct rte_ether_addr src_mac_addr,
                           struct rte_ether_addr dst_mac_addr,
                           rte_be32_t src_ip_addr,
                           rte_be32_t dst_ip_addr,
                           uint16_t src_port,
                           uint16_t dst_port);

// Initialzie a port with the number of tx/rx rings and the ring size
static int port_init(uint16_t port, struct rte_mempool *mbuf_pool,
                    uint16_t nb_tx_rings, uint16_t nb_rx_rings,
                    uint16_t tx_desc_size, uint16_t rx_desc_size);

// RX processing main loop function
static int lcore_rx_worker(void *arg);
static int lcore_feedback(void *arg);

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

static void print_config() {
    printf("================ Configuration ================\n");
    printf("Port:           %u\n", port_id);
    printf("Interval:       %u sec\n", interval);
    printf("===============================================\n");
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

// Generate packet
static void generate_packet(struct rte_mbuf *pkt,
                           uint16_t pkt_len,
                           struct rte_ether_addr src_mac_addr,
                           struct rte_ether_addr dst_mac_addr,
                           rte_be32_t src_ip_addr,
                           rte_be32_t dst_ip_addr,
                           uint16_t src_port,
                           uint16_t dst_port)
{
    if (unlikely(pkt == NULL)) { 
        printf("mbuf is not valid\n");
    }

    uint16_t ip_pkt_len = pkt_len - sizeof(struct rte_ether_hdr);
    uint16_t udp_pkt_len = ip_pkt_len - sizeof(struct rte_ipv4_hdr);

    // Initialize Ethernet header
    struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
    // eth_hdr->src_addr = src_mac_addr;
    // eth_hdr->dst_addr = dst_mac_addr;
    eth_hdr->s_addr = src_mac_addr;
    eth_hdr->d_addr = dst_mac_addr;
    eth_hdr->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

    // Initialize the IPv4 header
    struct rte_ipv4_hdr *ip_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_ipv4_hdr*, sizeof(struct rte_ether_hdr));
    ip_hdr->src_addr = src_ip_addr;
    ip_hdr->dst_addr = dst_ip_addr;
    ip_hdr->version_ihl = RTE_IPV4_VHL_DEF;
    ip_hdr->type_of_service = 2;
    ip_hdr->total_length = rte_cpu_to_be_16(ip_pkt_len);
    ip_hdr->packet_id = rte_cpu_to_be_16(0);
    ip_hdr->fragment_offset = 0;
    ip_hdr->time_to_live = 64;
    ip_hdr->next_proto_id = IPPROTO_UDP;
    ip_hdr->hdr_checksum = 0;
    ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);

    // Initialize the UDP header
    struct rte_udp_hdr *udp_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_udp_hdr*, 
                                    sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
    udp_hdr->src_port = rte_cpu_to_be_16(src_port);
    udp_hdr->dst_port = rte_cpu_to_be_16(dst_port);
    udp_hdr->dgram_len = rte_cpu_to_be_16(udp_pkt_len);
    udp_hdr->dgram_cksum = rte_ipv4_udptcp_cksum(ip_hdr, udp_hdr);

    pkt->data_len = pkt_len;
    pkt->pkt_len = pkt->data_len;
}

// Parse arguments
static int parse_port(char *arg) {
    long n;
    char **endptr;
    n = (uint16_t)strtol(arg, endptr, 10);
    if (n < 0 || n >= rte_eth_dev_count_avail()) {
        fprintf(stderr, "Invalid port\n");
        return -1;
    }
    port_id = (uint16_t)n;
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

static int parse_pkt_size(char *arg) {
    uint16_t n;
    char **endptr;
    n = (uint16_t)strtoul(arg, endptr, 10);
    if (n < RTE_ETHER_MIN_LEN - RTE_ETHER_CRC_LEN) {
        fprintf(stderr, "packet size should larger than %u\n", 
                RTE_ETHER_MIN_LEN - RTE_ETHER_CRC_LEN);
        return -1;
    } else if (n > RTE_ETHER_MAX_LEN - RTE_ETHER_CRC_LEN) {
        fprintf(stderr, "packet size should smaller than %u\n", 
                RTE_ETHER_MAX_LEN - RTE_ETHER_CRC_LEN);
        return -1;
    }
    pkt_size = n;
    return 0;
}

static int parse_latency(char *arg) {
    uint32_t n;
    char **endptr;
    n = (uint32_t)strtoul(arg, endptr, 10);
    if (n <= 0) {
        fprintf(stderr, "Invalid latency size, should be positve\n");
        return -1;
    }
    latency_size = n;
    return 0;
}

static int parse_test_pps(char *arg)
{
    uint32_t n;
    char **endptr;
    n = (uint32_t)strtoul(arg, endptr, 10);
    test_pps = n;
    return 0;
}

static int parse_app_id(char *arg)
{
    uint32_t n;
    char **endptr;
    n = (uint32_t)strtoul(arg, endptr, 10);
    if (n < 0) {
        fprintf(stderr, "Invalid application id\n");
        return -1;
    }
    app_id = n;
    return 0;
}

static int parse_app_arg1(char *arg)
{
    char **endptr;
    app_arg1 = (uint32_t)strtoul(arg, endptr, 10);
    return 0;
}

static int parse_app_arg2(char *arg)
{
    char **endptr;
    app_arg2 = (uint32_t)strtoul(arg, endptr, 10);
    return 0;
}

static int parse_dynamic_num_cores(char *arg)
{
    uint32_t n;
    char **endptr;
    n = (uint32_t)strtoul(arg, endptr, 10);
    if (n < 0) {
        fprintf(stderr, "Number of dyncamic cores cannot be negative\n");
        return -1;
    }
    dynamic_num_cores = n;
    return 0;
}

static int parse_send_feedback(char *arg)
{
    char **endptr;
    send_feedback = (uint32_t)strtoul(arg, endptr, 10);
    return 0;
}

static void print_usage(const char *prgname)
{
    printf("%s [EAL options] -- [-p PORT] [-i INTERVAL] [-r RATE]\n"
           "    -p, --port              port to receive packets (default %hu)\n"
           "    -i, --interval          stats retrival period in second (default %u)\n"
           "    -s, --size              packet payload size\n"
           "    -l, --latency           test latency size (default %u) disable it by set to 0\n"
           "    -t, --test-pps          whether record pps, enable=1, disable=0 (default enabled)\n"
           "    -a, --app-id            application id\n"
           "    -b, --app-arg1          first application argument\n"
           "    -c, --app-arg2          second application argument\n"
           "    -d, --dynamic           dynamic allocate number of worker cores, disabled=0 (default disabled)\n"
           "    -f, --feedback          send feedback to FPGA\n"
           "    -h, --help              print usage of the program, enable=1, disable=0 (default disabled)\n",
           prgname, port_id, interval, latency_size);
}

static struct option long_options[] = {
    {"port",        required_argument,  0,  'p'},
    {"interval",    required_argument,  0,  'i'},
    {"size",        required_argument,  0,  's'},
    {"latency",     required_argument,  0,  'l'},
    {"rate",        required_argument,  0,  'r'},
    {"test-pps",    required_argument,  0,  't'},
    {"app-id",      required_argument,  0,  'a'},
    {"app-arg1",    required_argument,  0,  'b'},
    {"app-arg2",    required_argument,  0,  'c'},
    {"dynamic",     required_argument,  0,  'd'},
    {"feedback",    required_argument,  0,  'f'},
    {"help",        no_argument,        0,  'h'},
    {0,             0,                  0,  0 }
};

static int parse_args(int argc, char **argv)
{
    char *prgname = argv[0];
    const char short_options[] = "p:i:s:l:r:t:a:b:c:d:f:h";
    int c;
    int ret;

    while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != EOF) {
        switch (c) {
            case 'p':
                ret = parse_port(optarg);
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

            case 's':
                ret = parse_pkt_size(optarg);
                if (ret < 0) {
                    return -1;
                }
                break;

            case 'l':
                ret = parse_latency(optarg);
                if (ret < 0) {
                    return -1;
                }
                break;

            case 't':
                ret = parse_test_pps(optarg);
                if (ret < 0) {
                    printf("Failed to parse pkt rate\n");
                    return -1;
                }
                break;

            case 'a':
                ret = parse_app_id(optarg);
                if (ret < 0) {
                    printf("Failed to parse pkt rate\n");
                    return -1;
                }
                break;
            
            case 'b':
                ret = parse_app_arg1(optarg);
                if (ret < 0) {
                    printf("Failed to parse pkt rate\n");
                    return -1;
                }
                break;

            case 'c':
                ret = parse_app_arg2(optarg);
                if (ret < 0) {
                    printf("Failed to parse pkt rate\n");
                    return -1;
                }
                break;

            case 'd':
                ret = parse_dynamic_num_cores(optarg);
                if (ret < 0) {
                    printf("Failed to parse dynamic_num_cores\n");
                    return -1;
                }
                break;

            case 'f':
                ret = parse_send_feedback(optarg);
                if (ret < 0) {
                    printf("Failed to parse send_feedback\n");
                    return -1;
                }
                break;

            case 'h':
            default:
                print_usage(prgname);
                return -1;
        }
    }

    if (optind >= 0) {
        argv[optind-1] = prgname;
    }
    optind = 1;

	return 0;
}

#endif /* _MAIN_H_ */
