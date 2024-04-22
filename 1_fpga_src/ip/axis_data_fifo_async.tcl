create_ip -name axis_data_fifo -vendor xilinx.com -library ip -version 2.0 -module_name axis_data_fifo_async

set_property -dict [list \
    CONFIG.Component_Name {axis_data_fifo_async} \
    CONFIG.TDATA_NUM_BYTES {64} \
    CONFIG.TUSER_WIDTH {17} \
    CONFIG.FIFO_DEPTH {512} \
    CONFIG.FIFO_MODE {2} \
    CONFIG.IS_ACLK_ASYNC {1} \
    CONFIG.HAS_TKEEP {1} \
    CONFIG.HAS_TLAST {1} \
    CONFIG.HAS_WR_DATA_COUNT {0} \
    CONFIG.HAS_AFULL {0}
] [get_ips axis_data_fifo_async]