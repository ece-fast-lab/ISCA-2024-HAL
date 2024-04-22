/*
    Running Software Load Balancer
*/

#include "main.h"

// RX packet processing main loop function
static int lcore_rx_worker()
{
    unsigned int lcore_id = rte_lcore_id();
    unsigned int lcore_index = rte_lcore_index(lcore_id);
    unsigned int rx_core_index = lcore_index;
    unsigned int tx_core_index = lcore_index;
    unsigned int rx_lcore_count = rte_lcore_count();

    printf("lcore %2u (lcore index %2u) starts to receive packets\n",
           lcore_id, lcore_index);

    struct rte_mbuf *bufs[BURST_SIZE];
    struct rte_mbuf *lat_bufs;

    uint64_t hz = rte_get_timer_hz();
    uint64_t interval_cycles = interval * hz;
    uint64_t micro_sec_cycles = (uint64_t) (interval_cycles / 1000.0); // 1ms
    printf("micro_sec_cycles: %lu\n", micro_sec_cycles);
    uint64_t cap_per_core = (uint64_t) (smartnic_cap / split_num_core / 1000.0); // 1ms
    printf("cap_per_core: %lu\n", cap_per_core);

    uint64_t prev_tsc, cur_tsc, diff_tsc;
    uint16_t nb_tx = 0;

    // test pps
    uint16_t pps_index = 0;
    uint64_t pps_prev_tsc, pps_cur_tsc, pps_diff_tsc;
    pps_prev_tsc = rte_get_timer_cycles();
    uint64_t last_lcore_rx_pkts = 0;
    uint64_t last_lcore_tx_pkts = 0;

    uint16_t worker_index = 0;

    while (likely(keep_receiving)) {
        cur_tsc = rte_get_timer_cycles();
        diff_tsc = cur_tsc - prev_tsc;
        if (diff_tsc > micro_sec_cycles) {
            lcore_rx_pkts_split[rx_core_index] = 0;
            prev_tsc = cur_tsc;
        }

        uint16_t nb_rx = rte_eth_rx_burst(rx_port_id, rx_core_index, bufs, BURST_SIZE);
        if (unlikely(nb_rx == 0)) {
            continue;
        }

        lcore_rx_pkts[rx_core_index] += nb_rx;

        uint16_t direction = 0; // direction 0: SNIC, 1: Host, 2: SNIC/Host
        uint16_t nb_nic = 0;
        if (lcore_rx_pkts_split[rx_core_index] + nb_rx <= cap_per_core) {
            // accept by SNIC
            direction = 0;
        } else if (lcore_rx_pkts_split[rx_core_index] < cap_per_core && lcore_rx_pkts_split[rx_core_index] + nb_rx > cap_per_core) {
            // forward to SNIC/host
            direction = 2;
            nb_nic = cap_per_core - lcore_rx_pkts_split[rx_core_index];
        } else {
            // forward to host
            direction = 1;
        }
        lcore_rx_pkts_split[rx_core_index] += nb_rx;


        // test pps
        if (test_pps == 1) {
            pps_cur_tsc = rte_get_timer_cycles();
            pps_diff_tsc = pps_cur_tsc - pps_prev_tsc;
            if (pps_diff_tsc > hz) {
                if (pps_index >= 600) pps_index = 0;

                uint64_t total_rx_pkts_per_sec = (lcore_rx_pkts[lcore_index] - last_lcore_rx_pkts);
                last_lcore_rx_pkts = lcore_rx_pkts[lcore_index];
                pps_arr[lcore_index].rx_data[pps_index] = total_rx_pkts_per_sec / 1000000.0 / pps_diff_tsc * hz;

                uint64_t total_tx_pkts_per_sec = (lcore_tx_pkts[lcore_index] - last_lcore_tx_pkts);
                last_lcore_tx_pkts = lcore_tx_pkts[lcore_index];
                pps_arr[lcore_index].tx_data[pps_index] = total_tx_pkts_per_sec / 1000000.0 / pps_diff_tsc * hz;

                pps_prev_tsc = pps_cur_tsc;
                pps_index++;
                if (lcore_index == 0) {
                    pps_size = pps_index;
                }
            }
        }

        if (direction == 0) {
            // accept by smartnic
            // send to worker core using ring
            int ret;
            if (lcore_params_array[lcore_index].n_rings != 0) {
                ret = rte_ring_sp_enqueue_bulk(
                    lcore_params_array[lcore_index].rx_worker_ring[worker_index],
                    (void **) bufs,
                    nb_rx,
                    NULL);
                worker_index = (worker_index + 1) % lcore_params_array[lcore_index].n_rings;

                uint16_t buf;
                for (buf = ret; buf < nb_rx; buf++)
                    rte_pktmbuf_free(bufs[buf]);

            } else {
                uint16_t buf;
                for (buf = 0; buf < nb_rx; buf++)
                    rte_pktmbuf_free(bufs[buf]);
            }

        } else if (direction == 1) {
            // forward to host
            uint16_t nb_tx = 0;
            for (uint16_t j = 0; j < nb_rx; j++) {
                l2fwd_mac_updating(bufs[j]);
            }
            while (nb_tx < nb_rx){
                // Different TX/RX port
                nb_tx += rte_eth_tx_burst(tx_port_id, tx_core_index, bufs+nb_tx, nb_rx-nb_tx);
            }
            lcore_tx_pkts[tx_core_index] += nb_tx;

            // free unsent packets if any
            if (unlikely(nb_tx < nb_rx)) {
                uint16_t buf_to_free;
                for (buf_to_free = nb_tx; buf_to_free < nb_rx; buf_to_free++) {
                    rte_pktmbuf_free(bufs[buf_to_free]);
                }
            }
        } else {
            // forward to smartnic/host
            // send to worker core using ring
            int ret;
            if (lcore_params_array[lcore_index].n_rings != 0) {
                ret = rte_ring_sp_enqueue_bulk(
                    lcore_params_array[lcore_index].rx_worker_ring[worker_index],
                    (void **) bufs,
                    nb_nic,
                    NULL);
                worker_index = (worker_index + 1) % lcore_params_array[lcore_index].n_rings;

                uint16_t buf;
                for (buf = ret; buf < nb_nic; buf++)
                    rte_pktmbuf_free(bufs[buf]);
            } else {
                uint16_t buf;
                for (buf = 0; buf < nb_nic; buf++)
                    rte_pktmbuf_free(bufs[buf]);
            }


            uint16_t nb_tx = 0;
            for (uint16_t j = nb_nic; j < nb_rx; j++) {
                l2fwd_mac_updating(bufs[j]);
            }
            while (nb_tx < nb_rx - nb_nic){
                // Different TX/RX port
                nb_tx += rte_eth_tx_burst(tx_port_id, tx_core_index, bufs+nb_nic+nb_tx, nb_rx-nb_nic-nb_tx);
            }
            lcore_tx_pkts[tx_core_index] += nb_tx;

            // free unsent packets if any
            if (unlikely(nb_tx < nb_rx - nb_nic)) {
                uint16_t buf_to_free;
                for (buf_to_free = nb_nic; buf_to_free < nb_rx; buf_to_free++) {
                    rte_pktmbuf_free(bufs[buf_to_free]);
                }
            }
        }
    }

    return 0;
}


static int lcore_app_worker()
{
    unsigned int lcore_id = rte_lcore_id();
    unsigned int lcore_index = rte_lcore_index(lcore_id);
    unsigned int tx_core_index = lcore_index - split_num_core;

    printf("lcore %2u (lcore index %2u) starts to work\n",
           lcore_id, lcore_index);

    void *state;
    switch(app_id) {
        case 1:
            state = (struct _matchinfo *)malloc(sizeof(struct _matchinfo) * TOTAL_MATCH);
            bm25_init(state, app_arg1);
            break;
        case 2:
            state = malloc(sizeof(struct _bayes_state));
            bayes_init(state, app_arg1);
            break;
        case 3:
            state = malloc(sizeof(struct _knn_node) * app_arg1);
            knn_init(state, app_arg1);
            break;
        case 4:
            state = malloc(sizeof(struct _nat_entry) * app_arg1);
            nat_init(state, app_arg1);
            break;
        default:
            break;
    }

    int forward_index = 0;
    struct rte_mbuf *bufs[BURST_SIZE];

    uint64_t hz = rte_get_timer_hz();
    uint64_t interval_cycles = interval * hz;
    uint64_t lat_sec_cycles = (uint64_t) (interval_cycles / 1000.0); // 1ms

    uint64_t lat_prev_tsc, lat_cur_tsc, lat_diff_tsc;
    uint16_t lat_nb_tx = 0;
    lat_prev_tsc = rte_get_timer_cycles();
    
    // test pps
    uint16_t pps_index = 0;
    uint64_t pps_prev_tsc, pps_cur_tsc, pps_diff_tsc;
    pps_prev_tsc = rte_get_timer_cycles();
    uint64_t last_lcore_work_pkts = 0;

    while(likely(keep_receiving)) {

        int nb_rx;
        int nb_tx;
        int ret;

        if (lcore_params_array[lcore_index].n_rings == 0) {
            continue;
        }

        nb_rx = rte_ring_sc_dequeue_bulk(
            lcore_params_array[lcore_index].rx_worker_ring[forward_index],
            (void **) bufs,
            BURST_SIZE,
            NULL);
        lcore_work_pkts[lcore_index] += nb_rx;
        forward_index = (forward_index + 1) % lcore_params_array[lcore_index].n_rings;

        if (unlikely(nb_rx == 0)) {
            continue;
        }

        if (test_pps == 1) {
            pps_cur_tsc = rte_get_timer_cycles();
            pps_diff_tsc = pps_cur_tsc - pps_prev_tsc;
            if (pps_diff_tsc > hz) {
                if (pps_index >= 600) pps_index = 0;

                uint64_t total_work_pkts_per_sec = (lcore_work_pkts[lcore_index] - last_lcore_work_pkts);
                last_lcore_work_pkts = lcore_work_pkts[lcore_index];
                pps_arr[lcore_index].work_data[pps_index] = total_work_pkts_per_sec / 1000000.0 / pps_diff_tsc * hz;

                pps_prev_tsc = pps_cur_tsc;
                pps_index++;
            }
        }

        for (int i; i < nb_rx; i++) {
            uint32_t input_data;
            switch(app_id) {
                case 1:
                    bm25_exec(state);
                    break;
                case 2:
                    bayes_exec(state, app_arg1);
                    break;
                case 3:
                    knn_exec(state, app_arg1, app_arg2);
                    break;
                case 4:
                    nat_exec(state, app_arg1, input_data);
                    break;
                default:
                    break;
            }
        }

        // latency test
        // send back packets
        lat_cur_tsc = rte_get_timer_cycles();
        lat_diff_tsc = lat_cur_tsc - lat_prev_tsc;
        lat_nb_tx = 0;
        if (lat_diff_tsc > lat_sec_cycles) {
            struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(bufs[0], struct rte_ether_hdr *);
            rte_ether_addr_copy(&eth_hdr->s_addr, &eth_hdr->d_addr);
            rte_ether_addr_copy(&my_ether_addr, &eth_hdr->s_addr);
            lat_nb_tx = rte_eth_tx_burst(rx_port_id, tx_core_index, bufs, 1);
            lat_prev_tsc = lat_cur_tsc;
        }

        if (likely(lat_nb_tx < nb_rx)) {
            uint16_t buf;
            for (buf = lat_nb_tx; buf < nb_rx; buf++) {
                rte_pktmbuf_free(bufs[buf]);
            }
        }
    }

    switch(app_id) {
        case 1:
            bm25_free(state);
            break;
        case 2:
            bayes_free(state, app_arg1);
            break;
        case 3:
            knn_free(state);
            break;
        case 4:
            nat_free(state);
            break;
        default:
            break;
    }

    return 0;
}


static int lcore_main_loop()
{
    unsigned int lcore_id = rte_lcore_id();
    unsigned int lcore_index = rte_lcore_index(lcore_id);

    if (lcore_index < split_num_core) {
        printf("lcore %2u starts to receive packets\n", lcore_id);
        lcore_rx_worker();
    } else {
        printf("lcore %2u starts to work\n", lcore_id);
        lcore_app_worker();
    }

}


int main(int argc, char **argv)
{
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
    }

    printf("%d EAL arguments used\n", ret);

    argc -= ret;
    argv += ret;

    // Parse application arguments
    ret = parse_args(argc, argv);
    printf("%d application arguments used\n", ret);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Invalid application arguments\n");
    }

    print_config();

    // Init RTE timer library
    rte_timer_subsystem_init();

    unsigned int rx_lcore_count = rte_lcore_count();
    unsigned int tx_lcore_count = rte_lcore_count();
    printf("RX/TX lcore count: %u\n", rx_lcore_count);
    unsigned int nb_ports = rte_eth_dev_count_avail();
    printf("Number of available ports is %u\n", nb_ports);

    if (rx_lcore_count > MAX_RX_CORES) {
        rte_exit(EXIT_FAILURE, "Only %u lcores are available\n", MAX_RX_CORES);
    }

    for (size_t i = 0; i < rx_lcore_count; i++) {
        last_lcore_rx_pkts[i] = 0;
        lcore_rx_pkts[i] = 0;
        last_lcore_tx_pkts[i] = 0;
        lcore_tx_pkts[i] = 0;
        lcore_rx_pkts_split[i] = 0;
    }

    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
                                                            NUM_MBUFS * rx_lcore_count,
		                                                    MBUF_CACHE_SIZE,
                                                            0,
                                                            RTE_MBUF_DEFAULT_BUF_SIZE,
                                                            rte_socket_id());

    if (!mbuf_pool) {
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
    }

    nb_worker_cores = rx_lcore_count - split_num_core;

    if (port_init(rx_port_id, mbuf_pool, nb_worker_cores, split_num_core, TX_DESC_DEFAULT, RX_DESC_DEFAULT) != 0) {
        rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n", rx_port_id);
    }

    if (port_init(tx_port_id, mbuf_pool, split_num_core, 0, TX_DESC_DEFAULT, RX_DESC_DEFAULT) != 0) {
        rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n", tx_port_id);
    }

    if (ring_init(rx_lcore_count, split_num_core) != 0) {
        rte_exit(EXIT_FAILURE, "Cannot init ring\n");
    }

    keep_receiving = 1;
    signal(SIGINT, stop_rx);
    
    uint64_t hz = rte_get_timer_hz();
    uint64_t interval_cycles = interval * hz;
    uint64_t prev_tsc, cur_tsc, diff_tsc;
    prev_tsc = rte_get_timer_cycles();

    // Launch logical cores
    rte_eal_mp_remote_launch(lcore_main_loop, NULL, CALL_MAIN);

    // Stop and cleanup
    ret = rte_eth_dev_stop(rx_port_id);
    if (ret != 0)
        printf("rte_eth_dev_stop: err=%d, port=%d\n", ret, rx_port_id);
    rte_eth_dev_close(rx_port_id);

    ret = rte_eth_dev_stop(tx_port_id);
    if (ret != 0)
        printf("rte_eth_dev_stop: err=%d, port=%d\n", ret, tx_port_id);
    rte_eth_dev_close(tx_port_id);

    rte_eal_cleanup();

    // Print statistics
    uint64_t total_rx_pkts = 0;
    uint64_t total_tx_pkts = 0;
    uint64_t total_work_pkts = 0;
    float pkt_rate = 0;
    float rx_pkt_rate = 0;
    float tx_pkt_rate = 0;
    float work_pkt_rate = 0;

    total_rx_pkts = 0;
    for (size_t i = 0; i < rx_lcore_count; i++) {
        total_rx_pkts += lcore_rx_pkts[i];
    }

    total_tx_pkts = 0;
    for (size_t i = 0; i < tx_lcore_count; i++) {
        total_tx_pkts += lcore_tx_pkts[i];
    }

    total_work_pkts = 0;
    for (size_t i = 0; i < rx_lcore_count; i++) {
        total_work_pkts += lcore_work_pkts[i];
    }

    cur_tsc = rte_get_timer_cycles();
    diff_tsc = cur_tsc - prev_tsc;

    rx_pkt_rate = (double) total_rx_pkts / diff_tsc * hz / 1000000.0;
    printf("Total RX packet per second: %0.4f M \n", rx_pkt_rate);
    tx_pkt_rate = (double) total_tx_pkts / diff_tsc * hz / 1000000.0;
    printf("Total TX packet per second: %0.4f M \n", tx_pkt_rate);
    work_pkt_rate = (double) total_work_pkts / diff_tsc * hz / 1000000.0;
    printf("Total work packet per second: %0.4f M \n", work_pkt_rate);


    printf("Receive %" PRId64 " packets in total\n", total_rx_pkts);
    for (size_t i = 0; i < rx_lcore_count; i++) {
        printf("RX ring %2lu receives %" PRId64 " packets (%0.3f%%)\n",
               i,
               lcore_rx_pkts[i],
               total_rx_pkts? lcore_rx_pkts[i] * 100.0 / total_rx_pkts : 0.0);
    }

    printf("Forward %" PRId64 " packets in total\n", total_tx_pkts);
    for (size_t i = 0; i < rx_lcore_count; i++) {
        printf("TX ring %2lu forwards %" PRId64 " packets (%0.3f%%)\n",
               i,
               lcore_tx_pkts[i],
               total_tx_pkts? lcore_tx_pkts[i] * 100.0 / total_tx_pkts : 0.0);
    }

    printf("Work %" PRId64 " packets in total\n", total_work_pkts);
    for (size_t i = 0; i < rx_lcore_count; i++) {
        printf("Work ring %2lu works %" PRId64 " packets (%0.3f%%)\n",
               i,
               lcore_work_pkts[i],
               total_work_pkts? lcore_work_pkts[i] * 100.0 / total_work_pkts : 0.0);
    }

    // Print pps stats
    if (test_pps == 1) {
        for(uint16_t i = 0; i < pps_size; i++) {
            double pps = 0;
            for (uint16_t j = 0; j < rx_lcore_count; j++) {
                pps += pps_arr[j].rx_data[i];
            }
            printf("Receiving packets at %u s is %f Mpps\n", i, pps);

            pps = 0;
            for (uint16_t j = 0; j < rx_lcore_count; j++) {
                pps += pps_arr[j].tx_data[i];
            }
            printf("Forwarding packets at %u s is %f Mpps\n", i, pps);

            pps = 0;
            for (uint16_t j = 0; j < rx_lcore_count; j++) {
                pps += pps_arr[j].work_data[i];
            }
            printf("Working packets at %u s is %f Mpps\n", i, pps);
        }
    } 

    return EXIT_SUCCESS;
}
