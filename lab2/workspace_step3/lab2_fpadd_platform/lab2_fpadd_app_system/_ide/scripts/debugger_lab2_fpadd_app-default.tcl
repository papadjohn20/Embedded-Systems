# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: /home/nbellas/workspace_step3/lab2_fpadd_platform/lab2_fpadd_app_system/_ide/scripts/debugger_lab2_fpadd_app-default.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source /home/nbellas/workspace_step3/lab2_fpadd_platform/lab2_fpadd_app_system/_ide/scripts/debugger_lab2_fpadd_app-default.tcl
# 
connect -url tcp:127.0.0.1:3121
targets -set -nocase -filter {name =~"APU*"}
rst -system
after 3000
targets -set -filter {jtag_cable_name =~ "Digilent Zed 210248BE70FD" && level==0 && jtag_device_ctx=="jsn-Zed-210248BE70FD-23727093-0"}
fpga -file /home/nbellas/workspace_step3/lab2_fpadd_platform/lab2_fpadd_app/_ide/bitstream/lab2_fpadd_wrapper.bit
targets -set -nocase -filter {name =~"APU*"}
loadhw -hw /home/nbellas/workspace_step3/lab2_fpadd_platform/lab2_fpadd_platform/export/lab2_fpadd_platform/hw/lab2_fpadd_wrapper.xsa -mem-ranges [list {0x40000000 0xbfffffff}] -regs
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"}
source /home/nbellas/workspace_step3/lab2_fpadd_platform/lab2_fpadd_app/_ide/psinit/ps7_init.tcl
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "*A9*#0"}
dow /home/nbellas/workspace_step3/lab2_fpadd_platform/lab2_fpadd_app/Debug/lab2_fpadd_app.elf
configparams force-mem-access 0
targets -set -nocase -filter {name =~ "*A9*#0"}
con
