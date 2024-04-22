# Traffic Receiver

## Arguments

- `-p, --port` - Port to receive packets (default `%hu`)
- `-i, --interval` - Stats retrieval period in seconds (default `%u`)
- `-s, --size` - Packet payload size
- `-l, --latency` - Test latency size (default `%u`); disable by setting to 0
- `-t, --test-pps` - Whether to record packets per second (pps), enable=1, disable=0 (default enabled)
- `-a, --app-id` - Application ID
- `-b, --app-arg1` - First application argument
- `-c, --app-arg2` - Second application argument
- `-d, --dynamic` - Dynamically allocate number of worker cores, disabled=0 (default disabled)
- `-f, --feedback` - Send feedback to FPGA
- `-h, --help` - Print usage of the program, enable=1, disable=0 (default disabled)