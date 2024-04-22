# Experiment Scripts

## Figure 2 and Figure 3
- Maximum throughput and p99 latency of the SNIC processor, normalized to those of host processor
- Average power consumption and energy efficiency (i.e., throughput/power) of a server using an SNIC processor, normalized to those of a server using the host processor

## Figure 4
- Throughput and p99 latency (top), and system-wide power consumption and energy efficiency (bottom) versus packet rate for running REM (left) and NAT (right) on the host and SNIC processors. The dotted lines represent data points that the SNIC processor notably increases p99 latency

## Figure 5
- Throughput and p99 latency of NAT with SLB. We sweep the number of SNIC CPU cores that process DPDK packets and run SLB from one to four, and FwdTh from 20 Gbps to 60 Gbps after making a client send packets to the SNIC at 80 Gbps.

## Figure 9
- Throughput, p99 latency, and power consumption across different network packet rates

## Common
- Environment setup
- Code compilation