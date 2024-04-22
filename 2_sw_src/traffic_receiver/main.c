/*
    Running Traffic Receiver with applications
*/

#include "main.h"

// RX packet processing main loop function
static int lcore_rx_worker(void *arg)
{
    unsigned int lcore_id = rte_lcore_id();
    unsigned int lcore_index = rte_lcore_index(lcore_id);

    printf("lcore %2u (lcore index %2u) starts to receive packets\n", lcore_id, lcore_index);

    // launch application
    void *state;
    struct _nat_entry *nat_state;
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
            nat_state = NULL;
            nat_init(&nat_state, app_arg1);
            break;
        case 9:
            nat_state = NULL;
            nat_init(&nat_state, app_arg1);
            break;
        case 10:
            nat_state = NULL;
            nat_init(&nat_state, app_arg1);
            break;
        default:
            break;
    }


    int dev_socket_id = rte_eth_dev_socket_id(port_id);
    if (dev_socket_id != (int)rte_socket_id()) {
        fprintf(stderr, "WARNING, port %u is on remote NUMA node to lcore %u\n", port_id, lcore_id);
    }

    struct rte_mbuf *bufs[BURST_SIZE];
    uint64_t hz = rte_get_timer_hz();

    // MODIFY HERE: TEST LATENCY PERIOD
    uint64_t interval_cycles = (uint64_t) (interval * hz / 1000.0); // 10ms

    uint64_t prev_tsc, cur_tsc, diff_tsc;
    prev_tsc = rte_get_timer_cycles();

    uint16_t nb_tx = 0;

    // test pps
    uint16_t pps_index = 0;
    uint64_t pps_prev_tsc, pps_cur_tsc, pps_diff_tsc;
    pps_prev_tsc = rte_get_timer_cycles();
    uint64_t last_lcore_rx_pkts = 0;

    size_t offset = sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr);
    

    unsigned int rx_index;
    if (send_feedback != 0) {
        rx_index = lcore_index - 1;
    } else {
        rx_index = lcore_index;
    }
    unsigned int tx_index = lcore_index;

    unsigned int core_more_hint = 0;
    unsigned int core_less_hint = 0;

    unsigned int traffic_more_hint = 0;
    unsigned int traffic_less_hint = 0;

    unsigned int rx_lcore_count = rte_lcore_count();

    struct rte_mbuf *tx_mbufs[BURST_SIZE];


    uint16_t src_port = rte_rand_max(65536);
    uint16_t dst_port = rte_rand_max(65536);

    uint64_t total_feedback_rx_pkts = 0;
    uint64_t prev_total_feedback_rx_pkts = 0;

    uint64_t feedback_split_rate = 0;
    int feedback_tx_sigal = 0;


    while (likely(keep_receiving)) {

        while (likely(keep_receiving) && lcore_params_array[lcore_index].working == 0){
            struct rte_epoll_event event[8];
            int n = rte_epoll_wait(RTE_EPOLL_PER_THREAD, event, 8, 100);
        }

        for (rx_index = lcore_params_array[lcore_index].rx_queues_start_index; rx_index < lcore_params_array[lcore_index].rx_queues_start_index + lcore_params_array[lcore_index].n_rx_queues; rx_index++) {
            if (lcore_params_array[lcore_index].working == 0) {
                break;
            }

            uint16_t nb_rx;
            nb_rx = rte_eth_rx_burst(port_id, rx_index, bufs, BURST_SIZE);

            if (unlikely(nb_rx == 0)) {
                // no packet received
                zero_rx_lcore_pkt_count[lcore_index] += 1;

                // rte_delay_us(100);

                if (zero_rx_lcore_pkt_count[lcore_index] > 100000 && send_feedback == 0) {
                    rte_eth_dev_rx_intr_enable(port_id, rx_index);
                    struct rte_epoll_event event[8];
                    int n = rte_epoll_wait(RTE_EPOLL_PER_THREAD, event, 8, 100);
                    rte_eth_dev_rx_intr_disable(port_id, rx_index);
                }
                continue;
            } else {
                // packet received
                zero_rx_lcore_pkt_count[lcore_index] = 0;
            }

            lcore_rx_pkts[lcore_index] += nb_rx;


            // ========= application start =========
            char *payload_isal[nb_rx];
            for (int i = 0; i < nb_rx; i++) {
                uint32_t *data = rte_pktmbuf_mtod_offset(bufs[i], uint32_t *, offset);
                uint32_t input_data = data[3];
                char *payload;

                if (app_id == 11) {
                    payload_isal[i] = (char *)calloc((size_t)1480, 1);
                    if (payload_isal[i] == NULL) {
                        printf("calloc failed test aborted\n");
                    }
                    memcpy(payload_isal[i], rte_pktmbuf_mtod_offset(bufs[i], char *, offset), (size_t)1480);
                }

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
                        nat_exec(nat_state, app_arg1, input_data);
                        break;
                    case 5:
                        kvs_exec(app_arg1, app_arg2, lcore_index, input_data);
                        break;
                    case 6:
                        count_exec(app_arg1, lcore_index, input_data);
                        break;
                    case 7:
                        ema_exec(app_arg1);
                        break;
                    case 8:
                        payload = rte_pktmbuf_mtod_offset(bufs[i], char *, offset);
                        crypto_exec(app_arg1, payload, lcore_index);
                        break;
                    case 9:
                        nat_exec(nat_state, app_arg1, input_data);
                        payload = rte_pktmbuf_mtod_offset(bufs[i], char *, offset);
                        crypto_exec(5, payload, lcore_index);
                        break;
                    case 10:
                        nat_exec(nat_state, app_arg1, input_data);
                        count_exec(8, lcore_index, input_data);
                        break;
                    default:
                        break;
                }
            }
            // ========= application end =========

            // dynamic num cores based on rx queue status
            if (dynamic_num_cores != 0) {
                cur_tsc = rte_get_timer_cycles();
                diff_tsc = cur_tsc - prev_tsc;

                if (diff_tsc > interval_cycles) {
                    if (lcore_index == 0) {

                        // check rx queue status
                        uint32_t rxq_count;
                        uint32_t max_rxq_count = 0; // ave
                        uint32_t max_rxq_id = 0;
                        uint32_t total_rxq_count = 0;
                        for (int i = 0; i < NUM_RX_QUEUES; i++) {
                            rxq_count = rte_eth_rx_queue_count(port_id, i);
                            total_rxq_count += rxq_count;
                            if (rxq_count > max_rxq_count) {
                                max_rxq_count = rxq_count;
                                max_rxq_id = i;
                            }
                        }

                        // change core hints based on rx queue status
                        if (dynamic_num_cores < rte_lcore_count()) {
                            if (max_rxq_count > CORE_MORE_WM_1 * BURST_SIZE) {
                                core_more_hint += CORE_MORE_STEP_1;
                            } else if (max_rxq_count > CORE_MORE_WM_2 * BURST_SIZE) {
                                core_more_hint += CORE_MORE_STEP_2;
                            } else {
                                core_more_hint = 0;
                            }
                        } else {
                            core_more_hint = 0;
                        }

                        if (max_rxq_count < CORE_LESS_WM_1 * BURST_SIZE && dynamic_num_cores != 1) {
                            core_less_hint += CORE_LESS_STEP_1;
                        } else {
                            core_less_hint = 0;
                        }

                        int quotient;
                        int remainder;
                        // add one core to working status
                        if (core_more_hint > CORE_CHANGE_WM) {
                            core_more_hint = 0;
                            core_less_hint = 0;
                            dynamic_num_cores += 1;

                            // assign rx queues to lcores
                            quotient = NUM_RX_QUEUES / dynamic_num_cores;
                            remainder = NUM_RX_QUEUES % dynamic_num_cores;

                            for (unsigned int i = 0; i < dynamic_num_cores; i++) {
                                lcore_params_array[i].n_rx_queues = quotient + (i < remainder ? 1 : 0);;
                                lcore_params_array[i].rx_queues_start_index = i * quotient + (i < remainder ? i : remainder);
                            }

                            rte_delay_us(10);

                            lcore_params_array[dynamic_num_cores - 1].working = 1;

                            printf("dynamic_num_cores: %d\n", dynamic_num_cores);
                        }

                        // remove one core from working status
                        if (core_less_hint > CORE_CHANGE_WM) {
                            core_more_hint = 0;
                            core_less_hint = 0;
                            lcore_params_array[dynamic_num_cores - 1].working = 0;
                            dynamic_num_cores -= 1;

                            rte_delay_us(10);

                            // assign rx queues to lcores
                            quotient = NUM_RX_QUEUES / dynamic_num_cores;
                            remainder = NUM_RX_QUEUES % dynamic_num_cores;

                            for (unsigned int i = 0; i < dynamic_num_cores; i++) {
                                lcore_params_array[i].n_rx_queues = quotient + (i < remainder ? 1 : 0);;
                                lcore_params_array[i].rx_queues_start_index = i * quotient + (i < remainder ? i : remainder);
                            }

                            printf("dynamic_num_cores: %d\n", dynamic_num_cores);
                        }
                    }
                }
            }


            if (latency_size != 0) {
                // latency test, send back packets
                cur_tsc = rte_get_timer_cycles();
                diff_tsc = cur_tsc - prev_tsc;
                nb_tx = 0;
                if (diff_tsc > interval_cycles) {
                    prev_tsc = cur_tsc;
                    struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(bufs[0], struct rte_ether_hdr *);
                    rte_ether_addr_copy(&eth_hdr->s_addr, &eth_hdr->d_addr);
                    rte_ether_addr_copy(&my_ether_addr, &eth_hdr->s_addr);
                    nb_tx = rte_eth_tx_burst(port_id, tx_index, bufs, 1);
                    lcore_tx_pkts[lcore_index] += nb_tx;
                }
                
                if (likely(nb_tx < nb_rx)) {
                    uint16_t buf;
                    for (buf = nb_tx; buf < nb_rx; buf++)
                        rte_pktmbuf_free(bufs[buf]);
                }
            } else {
                for (uint16_t i = 0; i < nb_rx; i++) {
                    rte_pktmbuf_free(bufs[i]);
                }
            }


            if (test_pps == 1) {
                // pps test
                pps_cur_tsc = rte_get_timer_cycles();
                pps_diff_tsc = pps_cur_tsc - pps_prev_tsc;
                if (pps_diff_tsc > hz) {
                    if (pps_index >= MAX_TIME) pps_index = 0;
                    uint64_t total_rx_pkts_per_sec = (lcore_rx_pkts[lcore_index] - last_lcore_rx_pkts);
                    last_lcore_rx_pkts = lcore_rx_pkts[lcore_index];
                    pps_arr[lcore_index].data[pps_index] = total_rx_pkts_per_sec / 1000000.0 / pps_diff_tsc * hz;
                    pps_prev_tsc = pps_cur_tsc;
                    pps_index++;
                    if (send_feedback == 0) {
                        if (lcore_index == 0) {
                            pps_size = pps_index;
                        }
                    } else {
                        if (lcore_index == 1) {
                            pps_size = pps_index;
                        }
                    }
                }
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
            nat_free(&nat_state);
            break;
        case 9:
            nat_free(&nat_state);
            break;
        case 10:
            nat_free(&nat_state);
            break;
        default:
            break;
    }

    return 0;
}


static int lcore_feedback(void *arg)
{
    unsigned int lcore_id = rte_lcore_id();
    unsigned int lcore_index = rte_lcore_index(lcore_id);
    struct rte_mempool *mbuf_pool = (struct rte_mempool*)arg;

    unsigned int traffic_more_hint = 0;
    unsigned int traffic_less_hint = 0;

    unsigned int rx_lcore_count = rte_lcore_count() - 1;
    struct rte_mbuf *tx_mbufs[BURST_SIZE];

    uint16_t src_port = rte_rand_max(65536);
    uint16_t dst_port = rte_rand_max(65536);

    uint64_t total_feedback_rx_pkts = 0;
    uint64_t prev_total_feedback_rx_pkts = 0;



    uint64_t hz = rte_get_timer_hz();
    uint64_t interval_cycles = (uint64_t) (interval * hz / 1000.0);
    uint64_t prev_tsc, cur_tsc, diff_tsc;
    prev_tsc = rte_get_timer_cycles();

    unsigned int tx_index = lcore_index;
    size_t offset = sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr);

    // Generate packets
    if (rte_pktmbuf_alloc_bulk(mbuf_pool, tx_mbufs, BURST_SIZE) != 0) {
        fprintf(stderr, "Fail to allocate %u mbufs on lcore %2u\n", BURST_SIZE, lcore_id);
        return -1;
    }

    uint64_t feedback_split_rate = 0;
    int feedback_tx_sigal = 0;

    uint32_t array[2];
    array[0] = 1717921125; // 0x66656565
    array[1] = send_feedback;
    generate_packet(tx_mbufs[0], 128, src_mac, dst_mac, src_ip, dst_ip, src_port, dst_port);
    char* payload = rte_pktmbuf_mtod_offset(tx_mbufs[0], char *, offset);
    rte_memcpy(payload, &array[0], sizeof(uint32_t)); 
    rte_memcpy(payload + sizeof(uint32_t), &array[1], sizeof(uint32_t)); 
    uint64_t nb_tx = rte_eth_tx_burst(port_id, tx_index, tx_mbufs, 1);
    if (nb_tx == 0) {
        printf("Fail to send feedback packet on lcore %u\n", lcore_id);
        rte_pktmbuf_free(tx_mbufs[0]);
    }

    uint64_t pps_prev_tsc, pps_cur_tsc, pps_diff_tsc;
    pps_prev_tsc = rte_get_timer_cycles();

    while (likely(keep_receiving)) {
        cur_tsc = rte_get_timer_cycles();
        diff_tsc = cur_tsc - prev_tsc;
        if (diff_tsc > interval_cycles) {
            prev_tsc = cur_tsc;
            nb_tx = 0;
            if (lcore_index == 0 && send_feedback != 0) {
                // check rx queue status
                uint32_t rxq_count;
                uint32_t ave_rxq_count = 0; // ave
                uint32_t max_rxq_count = 0; // max
                uint32_t total_rxq_count = 0;

                total_feedback_rx_pkts = 0;
                for (int i = 1; i < rx_lcore_count + 1; i++) {
                    rxq_count = rte_eth_rx_queue_count(port_id, i-1);
                    if (rxq_count > max_rxq_count) {
                        max_rxq_count = rxq_count;
                    }
                    total_feedback_rx_pkts += lcore_rx_pkts[i];
                    total_rxq_count += rxq_count;
                }

                double tp = 8.0 * (double)(pkt_size + 24.0) * (double)(total_feedback_rx_pkts - prev_total_feedback_rx_pkts) / (double)(diff_tsc) * (double)hz / 1000000000.0;
                prev_total_feedback_rx_pkts = total_feedback_rx_pkts;
                uint32_t fpga_tp = tp * 1000000000 / 10000 / (pkt_size + 24) * pkt_size / 8;

                ave_rxq_count = total_rxq_count / rx_lcore_count;
                ave_rxq_count = max_rxq_count;

                // change traffic hints based on rx queue status
                if (feedback_split_rate < HINT_MORE_DELTA + fpga_tp) {
                    if (ave_rxq_count == HINT_MORE_WM_1) {
                        traffic_more_hint += HINT_MORE_STEP_1;
                    } else if (ave_rxq_count < HINT_MORE_WM_2 * BURST_SIZE) {
                        traffic_more_hint += HINT_MORE_STEP_2;
                    } else if (ave_rxq_count < HINT_MORE_WM_3 * BURST_SIZE) {
                        traffic_more_hint += HINT_MORE_STEP_3;
                    } else if (ave_rxq_count < HINT_MORE_WM_4 * BURST_SIZE) {
                        traffic_more_hint += HINT_MORE_STEP_4;
                    } else {
                        traffic_more_hint = 0;
                    }
                } else {
                    traffic_more_hint = 0;
                }

                if (feedback_split_rate < HINT_LESS_DELTA + fpga_tp) {
                    if (ave_rxq_count > HINT_LESS_WM_1 * BURST_SIZE) {
                        traffic_less_hint += HINT_LESS_STEP_1;
                    } else if (ave_rxq_count > HINT_LESS_WM_2 * BURST_SIZE) {
                        traffic_less_hint += HINT_LESS_STEP_2;
                    } else if (ave_rxq_count > HINT_LESS_WM_3 * BURST_SIZE) {
                        traffic_less_hint += HINT_LESS_STEP_3;
                    } else {
                        traffic_less_hint = 0;
                    }
                } else {
                    traffic_less_hint = 0;
                }

                // change feedback rate based on traffic hints
                if (traffic_more_hint > FEEDBACK_MORE_WM) {
                    feedback_split_rate += FEEDBACK_STEP;
                    feedback_tx_sigal = 1;
                    // printf("more traffic\n");
                }

                if (traffic_less_hint > FEEDBACK_LESS_WM) {
                    if (feedback_split_rate > FEEDBACK_STEP) {
                        feedback_split_rate -= FEEDBACK_STEP;
                    } else {
                        feedback_split_rate = 1;
                    }
                    feedback_tx_sigal = 1;
                    // printf("less traffic\n");
                }

                if (feedback_tx_sigal == 1) {
                    traffic_more_hint = 0;
                    traffic_less_hint = 0;
                    feedback_tx_sigal = 0;
                    array[0] = 1717921125; // 0x66656565
                    if (send_feedback == 1) {
                        array[1] = feedback_split_rate;
                    } else {
                        array[1] = send_feedback;
                    }

                    generate_packet(tx_mbufs[0], 128, src_mac, dst_mac, src_ip, dst_ip, src_port, dst_port);
                    char* payload = rte_pktmbuf_mtod_offset(tx_mbufs[0], char *, offset);
                    rte_memcpy(payload, &array[0], sizeof(uint32_t)); 
                    rte_memcpy(payload + sizeof(uint32_t), &array[1], sizeof(uint32_t)); 
                    nb_tx = rte_eth_tx_burst(port_id, tx_index, tx_mbufs, 1);
                    if (nb_tx == 0) {
                        printf("Fail to send feedback packet on lcore %u\n", lcore_id);
                        rte_pktmbuf_free(tx_mbufs[0]);
                    }
                }
            }
        }

        if (test_pps == 1) {
            pps_cur_tsc = rte_get_timer_cycles();
            pps_diff_tsc = pps_cur_tsc - pps_prev_tsc;
            if (pps_diff_tsc > hz) {
                pps_prev_tsc = pps_cur_tsc;
                printf("feedback T: %lu bytes/100us\n", feedback_split_rate);
            }
        }
    }

    return 0;
}


static int lcore_main_loop(void *arg)
{
    unsigned int lcore_id = rte_lcore_id();
    unsigned int lcore_index = rte_lcore_index(lcore_id);

    if (lcore_index == 0 && send_feedback != 0) {
        printf("lcore %2u starts to send feedback\n", lcore_id);
        lcore_feedback(arg);
    } else {
        printf("lcore %2u starts to work\n", lcore_id);
        rte_delay_ms(1000);
        lcore_rx_worker(arg);
    }
}


int main(int argc, char **argv)
{
    // Initialize Environment Abstraction Layer (EAL)
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
    }
    printf("%d EAL arguments used\n", ret);

    argc -= ret;
    argv += ret;

    ret = parse_args(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Invalid application arguments\n");
    }
    print_config();

    // Init RTE timer library
    rte_timer_subsystem_init();

    // Get the number of RX lcores
    unsigned int rx_lcore_count = rte_lcore_count();
    printf("RX lcore count: %u\n", rx_lcore_count);
    if (rx_lcore_count > MAX_RX_CORES) {
        rte_exit(EXIT_FAILURE, "Only %u lcores are available\n", MAX_RX_CORES);
    }
    for (size_t i = 0; i < rx_lcore_count; i++) {
        last_lcore_rx_pkts[i] = 0;
        lcore_rx_pkts[i] = 0;
    }

    // Get the number of TX and RX queues
    unsigned int tx_queue_count = rte_lcore_count();
    unsigned int rx_queue_count;

    // Dynamic num cores should be between 0 and rte_lcore_count(), 0 means no dynamic
    printf("Dynamic num cores: %u\n", dynamic_num_cores); 
    if(dynamic_num_cores > rte_lcore_count() || dynamic_num_cores < 0) {
        rte_exit(EXIT_FAILURE, "Dynamic num cores should be between 0 and %u\n", rte_lcore_count());
    }
    if (dynamic_num_cores != 0) {
        rx_queue_count = NUM_RX_QUEUES;
    } else {
        rx_queue_count = rx_lcore_count;
    }
    printf("RX queue count: %u\n", rx_queue_count);

    // use 1 core to send feedback
    if (send_feedback != 0) {
        rx_queue_count -= 1;
    }

    // Create a memory pool
    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
                                                            NUM_MBUFS * rx_queue_count,
		                                                    MBUF_CACHE_SIZE,
                                                            0,
                                                            RTE_MBUF_DEFAULT_BUF_SIZE,
                                                            rte_socket_id());
    if (!mbuf_pool) {
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
    }

    // Initialize port
    if (port_init(port_id, mbuf_pool, tx_queue_count, rx_queue_count, TX_DESC_DEFAULT, RX_DESC_DEFAULT) != 0) {
        rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n", port_id);
    }


    // Initialize all lcore_params_array
    for (unsigned int i = 0; i < MAX_RX_CORES; i++) {
        lcore_params_array[i].n_rx_queues = 0;
        lcore_params_array[i].rx_queues_start_index = 0;
        lcore_params_array[i].working = 0;
    }

    // assign rx queues to lcores
    if (dynamic_num_cores != 0) {
        int quotient = NUM_RX_QUEUES / dynamic_num_cores;
        int remainder = NUM_RX_QUEUES % dynamic_num_cores;

        for (unsigned int i = 0; i < dynamic_num_cores; i++) {
            lcore_params_array[i].n_rx_queues = quotient + (i < remainder ? 1 : 0);;
            lcore_params_array[i].rx_queues_start_index = i * quotient + (i < remainder ? i : remainder);
            lcore_params_array[i].working = 1;
        }

        // print lcore_params_array
        for (unsigned int i = 0; i < dynamic_num_cores; i++) {
            printf("lcore %u: n_rx_queues: %u, rx_queues_start_index: %u\n", i, lcore_params_array[i].n_rx_queues, lcore_params_array[i].rx_queues_start_index);
        }

    } else {
        if (send_feedback != 0) {
            for (unsigned int i = 1; i < rx_lcore_count; i++) {
                lcore_params_array[i].n_rx_queues = 1;
                lcore_params_array[i].rx_queues_start_index = i-1;
                lcore_params_array[i].working = 1;
            }
        } else {
            for (unsigned int i = 0; i < rx_lcore_count; i++) {
                lcore_params_array[i].n_rx_queues = 1;
                lcore_params_array[i].rx_queues_start_index = i;
                lcore_params_array[i].working = 1;
            }
        }
    }

    // enable rx queue interrupts
    uint32_t data;
    for (unsigned int rx_index = 0; rx_index < rx_queue_count; rx_index++) {
        data = port_id << CHAR_BIT | rx_index;
        ret = rte_eth_dev_rx_intr_ctl_q(port_id, rx_index,
                    RTE_EPOLL_PER_THREAD,
                    RTE_INTR_EVENT_ADD,
                    (void *)((uintptr_t)data));
    }

    // Hardcoded MAC and IP addresses to send feedback
    if (rte_ether_unformat_addr("02:25:75:b2:d9:6b", &src_mac) < 0) {
        fprintf(stderr, "Invalid SRC_MAC %s\n", "02:25:75:b2:d9:6b");
        return -1;
    }

    if (rte_ether_unformat_addr("04:3f:72:9d:08:65", &dst_mac) < 0) {
        fprintf(stderr, "Invalid DST_MAC %s\n", "04:3f:72:9d:08:65");
        return -1;
    }

    if (inet_pton(AF_INET, "192.168.200.34", &src_ip) <= 0) {
        fprintf(stderr, "Invalid SRC_IP %s\n", "192.168.200.34");
        return -1;
    }

    if (inet_pton(AF_INET, "192.168.200.52", &dst_ip) <= 0) {
        fprintf(stderr, "Invalid DST_IP %s\n", "192.168.200.52");
        return -1;
    }

    // Ctrl+C handler
    keep_receiving = 1;
    signal(SIGINT, stop_rx);

    // Time measurement
    uint64_t hz = rte_get_timer_hz();
    uint64_t interval_cycles = interval * hz;
    uint64_t start_tsc, end_tsc, diff_tsc;
    uint64_t total_rx_pkts = 0;
    uint64_t total_tx_pkts = 0;
    start_tsc = rte_get_timer_cycles();
    float pkt_rate = 0;

    // Application init
    switch(app_id) {
        case 5:
            kvs_init();
            break;
        case 6:
            count_init();
            break;
        case 7:
            ema_init();
            break;
        case 8:
            crypto_init(app_arg2, app_arg1);
            break;
        case 9:
            crypto_init(app_arg2, 5);
            break;
        case 10:
            count_init();
            break;
        default:
            break;
    }

    rte_eal_mp_remote_launch(lcore_main_loop, mbuf_pool, CALL_MAIN);
    rte_eal_mp_wait_lcore();

    // Application free
    switch(app_id) {
        case 5:
            kvs_free();
            break;
        case 6:
            count_free();
            break;
        case 7:
            ema_free();
            break;
        case 8:
            crypto_free(app_arg1);
            break;
        case 9:
            crypto_free(5);
            break;
        case 10:
            count_free();
            break;
        default:
            break;
    }


    // Finishing program and print statistics
    total_rx_pkts = 0;
    for (size_t i = 0; i < rx_lcore_count; i++) {
        total_rx_pkts += lcore_rx_pkts[i];
    }
    end_tsc = rte_get_timer_cycles();
    diff_tsc = end_tsc - start_tsc;
    pkt_rate = (double) total_rx_pkts / diff_tsc * hz / 1000000.0;
    printf("Receive %" PRId64 " packets in total\n", total_rx_pkts);
    printf("Total RX packet per second: %0.4f M\n", pkt_rate);
    for (size_t i = 0; i < rx_lcore_count; i++) {
        printf("RX ring %2lu receives %" PRId64 " packets (%0.3f%%)\n",
               i, lcore_rx_pkts[i],
               total_rx_pkts ? lcore_rx_pkts[i] * 100.0 / total_rx_pkts : 0.0);
    }

    // Print pps stats every second for the latest MAX_TIME data
    if (test_pps == 1) {
        for(uint16_t i = 0; i < pps_size; i++) {
            double pps = 0;
            for (uint16_t j = 0; j < rx_lcore_count; j++) {
                pps += pps_arr[j].data[i];
            }
            printf("Receiving packets at %u s is %f Mpps\n", i, pps);
        }
    } 

    // Stop and cleanup
    ret = rte_eth_dev_stop(port_id);
    if (ret != 0)
        printf("rte_eth_dev_stop: err=%d, port=%d\n", ret, port_id);
    rte_eth_dev_close(port_id);
    rte_eal_cleanup();

    return EXIT_SUCCESS;
}
