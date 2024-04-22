/*
    Running Traffic Generator
*/

#include "main.h"

// TX packet generator main loop function
static int lcore_tx_worker(void *arg) {
    struct rte_mempool *mbuf_pool = (struct rte_mempool *)arg;
    struct rte_mbuf *mbufs[BURST_SIZE];

    unsigned int lcore_id = rte_lcore_id();
    unsigned int lcore_index = rte_lcore_index(lcore_id);
    unsigned int tx_index = lcore_index;

    if (lcore_id > rte_get_main_lcore()) {
        // skip main
        tx_index -= 1;
    }

    printf("lcore %2u (lcore index %2u, TX index %2u) starts to send packets\n",
            lcore_id, lcore_index, tx_index);

    // Performance warning on NUMA setup
    if (rte_eth_dev_socket_id(port) != (int)rte_socket_id()) {
        fprintf(stderr, "WARNING, NIC port %u is on remote NUMA node to lcore %u\n",
                port, lcore_id);
    }

    // Generate packets
    if (rte_pktmbuf_alloc_bulk(mbuf_pool, mbufs, BURST_SIZE) != 0) {
        fprintf(stderr, "Fail to allocate %u mbufs on lcore %2u\n", BURST_SIZE,
                lcore_id);
        return -1;
    }
    uint16_t max_refcnt_val = 65535;
    uint16_t cur_refcnt_val = max_refcnt_val;
    rte_srand(rte_get_timer_cycles());
    for (size_t i = 0; i < BURST_SIZE; i++) {
        uint16_t src_port = rte_rand_max(65536);
        uint16_t dst_port = rte_rand_max(65536);
        if (app_diff_ip == 0) {
            // send all to the same dest ip
            generate_packet(mbufs[i], pkt_size, src_mac, dst_mac, src_ip, dst_ip,
                            src_port, dst_port);
        } else {
            // give application different dest ip
            generate_packet(mbufs[i], pkt_size, src_mac, dst_mac,
                            app_dest_ip[tx_index], dst_ip, src_port, dst_port);
        }
        if (rte_mbuf_refcnt_read(mbufs[i]) != 1) {
            fprintf(stderr, "mbufs[%lu]'s refcnt should be 1\n", i);
            return -1;
        }
        rte_mbuf_refcnt_set(mbufs[i], max_refcnt_val);
    }

    uint64_t hz = rte_get_timer_hz();
    uint64_t interval_cycles = interval * hz;
    uint64_t prev_tsc, cur_tsc, diff_tsc;
    uint64_t trace_prev_tsc, trace_cur_tsc, trace_diff_tsc;
    uint64_t lat_test_start_time;
    char *payload;
    struct my_timestamp timestamp_data;
    size_t offset = sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) +
                    sizeof(struct rte_udp_hdr);
    uint16_t trace_step = 0;

    prev_tsc = rte_get_timer_cycles();
    trace_prev_tsc = rte_get_timer_cycles();

    // No rate limiting
    if (pkts_per_sec == 0) {
        while (likely(keep_sending)) {

            // Perform latency test, send packets with current timestamp
            if (latency_size != 0) {
                // add timestamp to packet contents
                lat_test_start_time = rte_get_timer_cycles();
                sprintf(timestamp_data.data, "%" PRIu64, lat_test_start_time);
                for (size_t i = 0; i < BURST_SIZE; i++) {
                    payload = rte_pktmbuf_mtod_offset(mbufs[i], char *, offset);
                    if (payload != NULL) {
                        rte_memcpy(payload, &timestamp_data.data, sizeof(struct my_timestamp));
                    }
                }
            }

            // Reset refcnt back to max_refcnt_val
            if (unlikely(cur_refcnt_val == 1)) {
                for (size_t i = 0; i < BURST_SIZE; i++) {
                    rte_mbuf_refcnt_set(mbufs[i], max_refcnt_val);
                }
            }

            size_t nb_tx_pkts = rte_eth_tx_burst(port, tx_index, mbufs, BURST_SIZE);
            lcore_tx_pkts[tx_index] += nb_tx_pkts;
            cur_refcnt_val--;
        }
    } else {
        // Rate limiting is enabled
        // Get the number of packets to send per lcore per second
        uint64_t hz = rte_get_timer_hz();
        uint32_t pps_per_lcore = pkts_per_sec / (rte_lcore_count() - 1);
        
        if (trace != 0) {
            // using data from trace
            pps_per_lcore = (csv_rates_array[trace_step] / (rte_lcore_count() - 1));
        }

        // Calculate credits a packet need to consume (using CPU cycles)
        uint64_t credits_per_pkt = hz / pps_per_lcore;
        if (unlikely(credits_per_pkt == 0)) {
            fprintf(stderr, "We cannot send more packets than # of cycles per second\n");
        }

        // Calculate credits for a burst of packets
        uint64_t max_credits = BURST_SIZE * credits_per_pkt;
        uint64_t credits = 0;
        uint64_t cur_tsc, prev_tsc = rte_get_timer_cycles();

        while (likely(keep_sending)) {
            // Reset refcnt back to max_refcnt_val
            if (unlikely(cur_refcnt_val == 1)) {
                for (size_t i = 0; i < BURST_SIZE; i++) {
                    rte_mbuf_refcnt_set(mbufs[i], max_refcnt_val);
                }
            }

            // Get current credit, skip sending if no sufficient credit
            cur_tsc = rte_get_timer_cycles();
            credits += cur_tsc - prev_tsc;
            prev_tsc = cur_tsc;
            if (credits < credits_per_pkt) {
                continue;
            }

            // Calculate the max number of packets can be sent 
            uint16_t max_nb_tx_pkts = credits / credits_per_pkt;
            if (max_nb_tx_pkts > BURST_SIZE) {
                max_nb_tx_pkts = BURST_SIZE;
            }

            // Perform latency test, send packets with current timestamp
            if (latency_size != 0) {
                // add timestamp to packet contents
                lat_test_start_time = rte_get_timer_cycles();
                sprintf(timestamp_data.data, "%" PRIu64, lat_test_start_time);
                for (size_t i = 0; i < max_nb_tx_pkts; i++) {
                    payload = rte_pktmbuf_mtod_offset(mbufs[i], char *, offset);
                    if (payload != NULL) {
                        rte_memcpy(payload, &timestamp_data.data, sizeof(struct my_timestamp));
                        // add random numbers to packet contents
                        for (size_t j = 0; j < 4; j++) {
                            uint32_t rand = rte_rand();
                            rte_memcpy(payload + sizeof(struct my_timestamp) + j * sizeof(uint32_t),
                                        &rand, sizeof(uint32_t));
                        }
                    }
                }
            }

            size_t nb_tx_pkts = rte_eth_tx_burst(port, tx_index, mbufs, max_nb_tx_pkts);
            lcore_tx_pkts[tx_index] += nb_tx_pkts;
            credits -= nb_tx_pkts * credits_per_pkt;
            if (credits > max_credits) {
                credits = max_credits;
            }

            if (trace != 0) {
                // Update credits per packet based on trace
                trace_cur_tsc = rte_get_timer_cycles();
                trace_diff_tsc = trace_cur_tsc - trace_prev_tsc;
                if (trace_diff_tsc > hz) {
                    trace_prev_tsc = trace_cur_tsc;
                    if (trace_step < RATE_NUM - 1) {
                        trace_step++;
                    } else {
                        trace_step = 0;
                    }
                    max_credits = BURST_SIZE * (hz / (csv_rates_array[trace_step] / (rte_lcore_count() - 1)));
                }
                credits_per_pkt = hz / (csv_rates_array[trace_step] / (rte_lcore_count() - 1));
                if (credits_per_pkt == 0) {
                    credits_per_pkt = 1;
                }
            }

            cur_refcnt_val--;
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    // Initialize Environment Abstraction Layer (EAL)
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
    }

    argc -= ret;
    argv += ret;

    // Parse application arguments
    ret = parse_args(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Invalid application arguments\n");
    }

    print_config();

    // Init RTE timer library
    rte_timer_subsystem_init();

    unsigned int tx_lcore_count = rte_lcore_count() - 1;
    printf("TX lcore count: %u\n", tx_lcore_count);

    if (tx_lcore_count > MAX_TX_CORES) {
        rte_exit(EXIT_FAILURE, "We only support up to %u lcores\n", MAX_TX_CORES);
    }

    // Assign different destination IP addresses for RSS
    if (app_diff_ip == 1) {
        for (int i = 0; i < tx_lcore_count; i++) {
            // IP address range
            char last[32];
            char domain[64] = "192.168.200.";
            sprintf(last, "%d", (60 + i));
            char *lcore_dest_ip = strcat(domain, last);
            ret = inet_pton(AF_INET, lcore_dest_ip, &app_dest_ip[i]);
        }
    }

    // Use real trace instead of fixed traffic rate
    if (trace != 0) {
        const char *trace_file_names[] = {"trace_1.csv", "trace_2.csv", "trace_3.csv"};
        int trace_file_index = trace - 1;
        float csv_rates_array_temp[RATE_NUM];
        if (trace_file_index < 0 || trace_file_index >= 3) {
            fprintf(stderr, "Invalid trace index\n");
            return -1;
        }

        FILE *fp = fopen(trace_file_names[trace_file_index], "r");
        if (fp == NULL) {
            fprintf(stderr, "Error reading file\n");
        }

        size_t trace_index = 0;
        while (fscanf(fp, "%f", &csv_rates_array_temp[trace_index]) == 1) {
            trace_index++;
        }

        fclose(fp);

        // Convert to packet rate
        for (int i = 0; i < RATE_NUM; i++) {
            csv_rates_array[i] = (double)csv_rates_array_temp[i] * 1000000000.0 /
                                (pkt_size + RTE_ETHER_CRC_LEN + 20.0) / 8.0;
        }
    }

    // Initialize tx pkt counters
    for (size_t i = 0; i < tx_lcore_count; i++) {
        last_lcore_tx_pkts[i] = 0;
        lcore_tx_pkts[i] = 0;
    }

    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL", NUM_MBUFS * tx_lcore_count, MBUF_CACHE_SIZE, 0,
        RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (!mbuf_pool) {
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
    }

    // Initialize port, configure 1 rx queue for latency test
    if (port_init(port, mbuf_pool, tx_lcore_count, 1, TX_DESC_DEFAULT, RX_DESC_DEFAULT) != 0) {
        rte_exit(EXIT_FAILURE, "Cannot init port %hu\n", port);
    }

    // Ctrl+C handler
    keep_sending = 1;
    signal(SIGINT, stop_tx);

    // skip main core for status printing
    rte_eal_mp_remote_launch(lcore_tx_worker, mbuf_pool, SKIP_MAIN);
  
    // Time measurement
    uint64_t hz = rte_get_timer_hz();
    uint64_t interval_cycles = interval * hz;
    uint64_t prev_tsc, cur_tsc, diff_tsc;
    uint64_t total_tx_pkts = 0;
    float pkt_rate = 0;
    float tput = 0;

    //
    uint64_t start_time, end_time, lat;
    struct rte_mbuf *bufs[BURST_SIZE];
    char *payload;
    prev_tsc = rte_get_timer_cycles();
    size_t offset = sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) +
                    sizeof(struct rte_udp_hdr);

    double *lat_arr = malloc(latency_size * sizeof(double));
    int step_index = 0;
    int latency_count = 0;

    // per sec latency
    int latency_size_per_sec = 1000;
    double *lat_arr_per_sec = malloc(latency_size_per_sec * sizeof(double));
    int count_per_sec = 0;
    int step_index_per_sec = 0;
    int full_flag = 0;

    // no latency measurement
    if (latency_size == 0) {
        while (likely(keep_sending)) {
            cur_tsc = rte_get_timer_cycles();
            diff_tsc = cur_tsc - prev_tsc;

            // Print statistics every second
            if (diff_tsc < interval_cycles) {
                continue;
            }

            prev_tsc = cur_tsc;
            total_tx_pkts = 0;
            for (size_t i = 0; i < tx_lcore_count; i++) {
                uint64_t tmp = lcore_tx_pkts[i];
                total_tx_pkts += (tmp - last_lcore_tx_pkts[i]);
                last_lcore_tx_pkts[i] = tmp;
            }

            // Print packet rate and throughput
            pkt_rate = total_tx_pkts * diff_tsc / 1000000.0 / interval_cycles;
            tput = pkt_rate * (pkt_size + RTE_ETHER_CRC_LEN + 20) * 8.0 / 1000.0;

            printf("Total TX packets per second: %8.4f M, throughput: %8.4f Gbps\n",
                    pkt_rate, tput);
        }
    } else {
        // with latency test
        while (likely(keep_sending)) {
            // receive a burst of packets and calculate latency
            lat = 0;
            end_time = rte_get_timer_cycles();
            uint16_t nb_rx_pkts = rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);
            if (unlikely(nb_rx_pkts == 0)) {
            } else {
                struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(bufs[0], struct rte_ether_hdr *);
                if (rte_is_same_ether_addr(&eth_hdr->dst_addr, &my_ether_addr)) { // disable it for REM tests
                    for (int i = 0; i < nb_rx_pkts; i++) {
                        payload = rte_pktmbuf_mtod_offset(bufs[i], char *, offset);
                        sscanf(payload, "%" SCNu64, &start_time);
                        lat += end_time - start_time;
                    }
                    // add to lat array
                    lat_arr[step_index] = lat * 1000000.0 / nb_rx_pkts / interval_cycles;
                    step_index++;
                    if (latency_count != latency_size) {
                        latency_count++;
                    }
                    if (step_index == latency_size - 1) {
                        step_index = 0;
                        latency_count = latency_size;
                    }

                    // add to per sec lat array
                    lat_arr_per_sec[step_index_per_sec] =
                        lat * 1000000.0 / nb_rx_pkts / interval_cycles;
                    step_index_per_sec++;
                    if (step_index_per_sec == latency_size_per_sec - 1) {
                        step_index_per_sec = 0;
                        full_flag = 1;
                    }

                    // only record the latency for the latest latency_size_per_sec stats
                    if (full_flag == 0) {
                        count_per_sec += 1;
                    } else {
                        count_per_sec = latency_size_per_sec;
                    }
                }
                // free buffers
                for (uint16_t i = 0; i < nb_rx_pkts; i++) {
                    rte_pktmbuf_free(bufs[i]);
                }
            }
            cur_tsc = rte_get_timer_cycles();
            diff_tsc = cur_tsc - prev_tsc;

            if (diff_tsc < interval_cycles) {
                continue;
            }

            prev_tsc = cur_tsc;
            total_tx_pkts = 0;
            for (size_t i = 0; i < tx_lcore_count; i++) {
                uint64_t tmp = lcore_tx_pkts[i];
                total_tx_pkts += (tmp - last_lcore_tx_pkts[i]);
                last_lcore_tx_pkts[i] = tmp;
            }

            // Print packet rate and throughput
            pkt_rate = total_tx_pkts * diff_tsc / 1000000.0 / interval_cycles;
            tput = pkt_rate * (pkt_size + RTE_ETHER_CRC_LEN + 20) * 8.0 / 1000.0;
            printf("Total TX packets per second: %8.4f M, throughput: %8.4f Gbps\n",
                    pkt_rate, tput);

            // Print per second 99 tail latency
            qsort(lat_arr_per_sec, count_per_sec, sizeof(double), cmpfunc);
            printf("99%% tail latency: %.2f us\n",
                    lat_arr_per_sec[(int)(0.99 * count_per_sec)]);
            full_flag = 0;
            count_per_sec = 0;
            step_index_per_sec = 0;
        }
    }

    // Stop and cleanup
    ret = rte_eth_dev_stop(port);
    if (ret != 0) {
        printf("rte_eth_dev_stop failed: err=%d, port=%d\n", ret, port);
    }
    rte_eth_dev_close(port);
    rte_eal_cleanup();

    // Print statistics
    total_tx_pkts = 0;
    for (size_t i = 0; i < tx_lcore_count; i++) {
        total_tx_pkts += lcore_tx_pkts[i];
    }

    printf("Send %" PRId64 " packets in total\n", total_tx_pkts);
    for (size_t i = 0; i < tx_lcore_count; i++) {
        printf("TX ring %2lu sends %" PRId64 " packets (%0.3f%%)\n", i,
            lcore_tx_pkts[i], lcore_tx_pkts[i] * 100.0 / total_tx_pkts);
    }

    // Print latency stats
    if (latency_size != 0) {
        double total_lat = 0.0;
        uint32_t valid_latency_size = 0;
        for (int i = 0; i < latency_count; i++) {
            total_lat += lat_arr[i];
            valid_latency_size++;
        }
        qsort(lat_arr, latency_count, sizeof(double), cmpfunc);
        printf("Total latency size: %d\n", latency_count);
        printf("Step index: %d\n", step_index);
        printf("Total latency: %.2f us\n", total_lat);
        printf("Valid latency size: %u\n", valid_latency_size);

        printf("Average latency: %.2f us\n",
            (double)total_lat / valid_latency_size);
        printf("min%% tail latency: %.2f us\n", lat_arr[(int)0]);
        printf("25%% tail latency: %.2f us\n",
            lat_arr[(int)(0.25 * latency_count)]);
        printf("50%% tail latency: %.2f us\n",
            lat_arr[(int)(0.50 * latency_count)]);
        printf("75%% tail latency: %.2f us\n",
            lat_arr[(int)(0.75 * latency_count)]);
        printf("90%% tail latency: %.2f us\n", lat_arr[(int)(0.9 * latency_count)]);
        printf("95%% tail latency: %.2f us\n",
            lat_arr[(int)(0.95 * latency_count)]);
        printf("99%% tail latency: %.2f us\n",
            lat_arr[(int)(0.99 * latency_count)]);
        printf("max%% tail latency: %.2f us\n", lat_arr[(int)(latency_count - 1)]);
    }

    // free latency arrays
    free(lat_arr_per_sec);
    free(lat_arr);

    return EXIT_SUCCESS;
}
