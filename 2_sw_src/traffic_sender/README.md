# Traffic Sender

## Arguments

- `-p, --port`             port to send packets (default `%hu`)
- `-i, --interval`         stats retrieval period in seconds (default `%u`)
- `-s, --size`             packet payload size
- `-l, --latency`          test latency size (default `%u`); disable it by setting to 0
- `-r, --packet-rate`      maximum packet sending rate in packets per second (no rate limiting by default)
- `-B, --source-mac`       source MAC address
- `-E, --dest-mac`         destination MAC address
- `-j, --source-ip`        source IP address
- `-J, --dest-ip`          destination IP address
- `-a, --app-diff-ip`      add different dest IP for different core, enable=1, disable=0 (default disable)
- `-b, --trace`            use trace, enable=1, disable=0 (default is disable)
- `-h, --help`             print usage of the program