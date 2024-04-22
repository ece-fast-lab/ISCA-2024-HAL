#ifndef _MAIN_H_
#define _MAIN_H_

#include <arpa/inet.h>
#include <getopt.h>
#include <math.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_random.h>
#include <rte_timer.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


// Configurable number of RX/TX ring descriptors
#define RX_DESC_DEFAULT 1024
#define TX_DESC_DEFAULT 1024

#define MTU_SIZE 1500

#define RATE_NUM 600

#define NUM_MBUFS 2047
#define MBUF_CACHE_SIZE 512
#define BURST_SIZE 64
#define MAX_TX_CORES 128

static const struct rte_eth_conf port_conf_default = {
        .txmode = {
            .mq_mode = RTE_ETH_MQ_TX_NONE
        }
};

struct my_timestamp {
    char data[64];
};

static uint16_t port = 0;
static uint16_t interval = 1;
static uint16_t pkt_size = RTE_ETHER_MIN_LEN - RTE_ETHER_CRC_LEN;
static struct rte_ether_addr src_mac;
static struct rte_ether_addr dst_mac;
static rte_be32_t src_ip;
static rte_be32_t dst_ip;
static uint32_t pkts_per_sec = 0;
static uint32_t latency_size = 100000000;
static uint16_t app_diff_ip = 0;
static rte_be32_t app_dest_ip[MAX_TX_CORES];
static uint16_t trace = 0;
static double csv_rates_array[RATE_NUM];

static struct rte_ether_addr my_ether_addr;

static uint64_t last_lcore_tx_pkts[MAX_TX_CORES];
static volatile uint64_t lcore_tx_pkts[MAX_TX_CORES];


// Compute latency stats
int cmpfunc(const void *a, const void *b);

// Calculate poisson
double next_poisson_time(double packet_rate, uint64_t tsc_frequency);

// Initialzie a port with the number of tx/rx rings and the ring size
static int port_init(uint16_t port, struct rte_mempool *mbuf_pool,
                    uint16_t nb_tx_rings, uint16_t nb_rx_rings,
                    uint16_t tx_desc_size, uint16_t rx_desc_size);

// Generate packet
static void generate_packet(struct rte_mbuf *pkt,
                           uint16_t pkt_len,
                           struct rte_ether_addr src_mac_addr,
                           struct rte_ether_addr dst_mac_addr,
                           rte_be32_t src_ip_addr,
                           rte_be32_t dst_ip_addr,
                           uint16_t src_port,
                           uint16_t dst_port);

// TX packet generator main loop function
static int lcore_tx_worker(void *arg);

// Keep sending packet until set to 0
static volatile int keep_sending = 1;
static void stop_tx(int sig) { keep_sending = 0; }

// Print functions
void print_mac(struct rte_ether_addr mac_addr) {
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
    printf("===================== Configuration =====================\n");
    printf("Port:                     %u\n", port);
    printf("Interval:             %u sec\n", interval);
    printf("Packet Size:        %u bytes\n", pkt_size);
    printf("Latency Size:             %u\n", latency_size);
    printf("Packet Rate:        ");
    if (pkts_per_sec == 0) {
        printf("N/A (no rate limiting)\n");
    } else {
        printf("%u\n", pkts_per_sec);
    }
    printf("Src MAC:                ");
    print_mac(src_mac);
    printf("Dst MAC:                ");
    print_mac(dst_mac);
    printf("=========================================================\n");
}


// Compute latency stats
int cmpfunc(const void *a, const void *b) {
    if (*(double *)a > *(double *)b)
        return 1;
    else if (*(double *)a < *(double *)b)
        return -1;
    else
        return 0;
}

// Calculate poisson
double next_poisson_time(double packet_rate, uint64_t tsc_frequency) {
    return -logf(1.0f - ((double)random()) / (double)(RAND_MAX)) /
                 (packet_rate / tsc_frequency);
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
    print_mac(addr);

    my_ether_addr = addr;

    // Enable RX in promiscuous mode for the Ethernet device.
    retval = rte_eth_promiscuous_enable(port);
    // End of setting RX port in promiscuous mode.
    if (retval != 0) {
        fprintf(stderr, "rte_eth_promiscuous_enable failed\n");
        return retval;
    }

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
    eth_hdr->src_addr = src_mac_addr;
    eth_hdr->dst_addr = dst_mac_addr;
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
    port = (uint16_t)n;
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

static int parse_pkt_rate(char *arg) {
    uint32_t n;
    char **endptr;
    n = (uint32_t)strtoul(arg, endptr, 10);
    if (n == 0) {
        fprintf(stderr, "Invalid packet rate, should be positve\n");
        return -1;
    }
    pkts_per_sec = n;
    return 0;
}

static int parse_src_mac(char *arg) {
    if (rte_ether_unformat_addr(arg, &src_mac) < 0) {
        fprintf(stderr, "Invalid SRC_MAC %s\n", arg);
        return -1;
    }
    return 0;
}

static int parse_dst_mac(char *arg) {
    if (rte_ether_unformat_addr(arg, &dst_mac) < 0) {
        fprintf(stderr, "Invalid DST_MAC %s\n", arg);
        return -1;
    }
    return 0;
}

static int parse_src_ip(char *arg) {
    if (inet_pton(AF_INET, arg, &src_ip) <= 0) {
        fprintf(stderr, "Invalid SRC_IP %s\n", arg);
        return -1;
    }
    return 0;
}

static int parse_dst_ip(char *arg) {
    if (inet_pton(AF_INET, arg, &dst_ip) <= 0) {
        fprintf(stderr, "Invalid DST_IP %s\n", arg);
        return -1;
    }
    return 0;
}

static int parse_app_diff_ip(char *arg) {
    uint32_t n;
    char **endptr;
    n = (uint32_t)strtoul(arg, endptr, 10);
    app_diff_ip = n;
    return 0;
}

static int parse_trace(char *arg) {
    uint32_t n;
    char **endptr;
    n = (uint32_t)strtoul(arg, endptr, 10);
    trace = n;
    return 0;
}

static void print_usage(const char *prgname) {
    printf("%s [EAL options] -- --options\n"
            "        -p, --port             port to send packets (default %hu)\n"
            "        -i, --interval         stats retrival period in second (default %u)\n"
            "        -s, --size             packet payload size\n"
            "        -l, --latency          test latency size (default %u) disable it by set to 0\n"
            "        -r, --packet-rate      maximum packet sending rate in packet per second (no rate limiting by default)\n"
            "        -B, --source-mac       source MAC address\n"
            "        -E, --dest-mac         destination MAC address\n"
            "        -j, --source-ip        source IP address\n"
            "        -J, --dest-ip          destination IP address\n"
            "        -a, --app-diff-ip      add different dest ip for different core, enable=1, disable=0 (default disable)\n"
            "        -b, --trace            use trace, enable=1, disable=0 (default is disable)\n"
            "        -h, --help             print usage of the program\n",
            prgname, port, interval, latency_size);
}

struct option long_options[] = {
    {"port",        required_argument,  0,  'p'},
    {"interval",    required_argument,  0,  'i'},
    {"size",        required_argument,  0,  's'},
    {"latency",     required_argument,  0,  'l'},
    {"rate",        required_argument,  0,  'r'},
    {"source-mac",  required_argument,  0,  'B'},
    {"dest-mac",    required_argument,  0,  'E'},
    {"source-ip",   required_argument,  0,  'j'},
    {"dest-ip",     required_argument,  0,  'J'},
    {"app-diff-ip", required_argument,  0,  'a'},
    {"trace",       required_argument,  0,  'b'},
    {"help",        no_argument,        0,  'h'},
    {0,             0,                  0,  0  }
};

static int parse_args(int argc, char **argv) {
    char short_options[] = "p:i:s:l:r:B:E:j:J:a:b:h";
    char *prgname = argv[0];
    int nb_required_args = 0;
    int retval;

    while (1) {
        int c = getopt_long(argc, argv, short_options, long_options, NULL);

        if (c == -1) {
            break;
        }

        switch (c) {
        case 'p':
            retval = parse_port(optarg);
            if (retval < 0) {
                return -1;
            }
            break;

        case 'i':
            retval = parse_interval(optarg);
            if (retval < 0) {
                return -1;
            }
            break;

        case 's':
            retval = parse_pkt_size(optarg);
            if (retval < 0) {
                return -1;
            }
            break;

        case 'l':
            retval = parse_latency(optarg);
            if (retval < 0) {
                return -1;
            }
            break;

        case 'r':
            retval = parse_pkt_rate(optarg);
            if (retval < 0) {
                return -1;
            }
            break;

        case 'B':
            retval = parse_src_mac(optarg);
            if (retval < 0) {
                return -1;
            }
            nb_required_args++;
            break;

        case 'E':
            retval = parse_dst_mac(optarg);
            if (retval < 0) {
                return -1;
            }
            nb_required_args++;
            break;

        case 'j':
            retval = parse_src_ip(optarg);
            if (retval < 0) {
                return -1;
            }
            break;

        case 'J':
            retval = parse_dst_ip(optarg);
            if (retval < 0) {
                return -1;
            }
            break;

        case 'a':
            retval = parse_app_diff_ip(optarg);
            if (retval < 0) {
                return -1;
            }
            break;

        case 'b':
            retval = parse_trace(optarg);
            if (retval < 0) {
                return -1;
            }
            break;

        case 'h':
        default:
            print_usage(prgname);
            return -1;
        }
    }

    if (nb_required_args != 2) {
        fprintf(stderr, "<source_mac> and <dest_mac> are required\n");
        print_usage(prgname);
        return -1;
    }

    if (optind >= 0) {
        argv[optind - 1] = prgname;
    }
    optind = 1;

    return 0;
}

#endif /* _MAIN_H_ */
