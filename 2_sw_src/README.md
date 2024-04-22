# HAL Software Implementation

## Components
- `traffic_receiver`: DPDK based traffic receiver
- `traffic_sender`: DPDK based traffic sender

- `software_load_balancer`: A load balancer is designed to determine whether packets received by the SNIC should be processed by the SNIC processor or forwarded to the host processor, depending on the rate of received packets (RateRx) and processing capability of the SNIC processor for a given function.
- `tools`: Tools to measure power and setup forward rate to FPGA