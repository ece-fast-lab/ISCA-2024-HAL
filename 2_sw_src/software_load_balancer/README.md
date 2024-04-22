# Software Load Balancer

## Arguments

- `-p, --rx-port` - RX port to receive packets (default `%hu`)
- `-q, --tx-port` - TX port to send packets (default `%hu`)
- `-i, --interval` - Stats retrieval period in seconds (default `%u`)
- `-r, --packet-rate` - Maximum packet sending rate in packets per second (no rate limiting by default)
- `-B, --source-mac` - Source MAC address
- `-E, --dest-mac` - Destination MAC address
- `-j, --source-ip` - Source IP address
- `-J, --dest-ip` - Destination IP address
- `-f, --forward` - Set mode to pure forwarding (1) or splitter (0) (default `%u`)
- `-c, --smartnic-cap` - Maximum capability of SmartNIC ARM cores in packets per second (pps)
- `-a, --app-id` - Application ID
- `-b, --app-arg1` - First application argument
- `-d, --app-arg2` - Second application argument
- `-s, --split_num_core` - Number of cores used for receiving packets
- `-h, --help` - Print usage of the program