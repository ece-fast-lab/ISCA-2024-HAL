create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.2 -module_name fifo_generator_async

set_property -dict [list \
  CONFIG.Component_Name {fifo_generator_async} \
  CONFIG.Fifo_Implementation {Independent_Clocks_Block_RAM} \
  CONFIG.INTERFACE_TYPE {Native} \
  CONFIG.Input_Data_Width {32} \
  CONFIG.Input_Depth {32} \
  CONFIG.Performance_Options {Standard_FIFO} \
  CONFIG.asymmetric_port_width {false} \
  CONFIG.synchronization_stages {3} \
] [get_ips fifo_generator_async]