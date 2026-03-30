# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct /home/nbellas/workspace2/lab2_interrupt_arm/lab2_interrupt_arm_platform/platform.tcl
# 
# OR launch xsct and run below command.
# source /home/nbellas/workspace2/lab2_interrupt_arm/lab2_interrupt_arm_platform/platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {lab2_interrupt_arm_platform}\
-hw {/home/nbellas/courses/ECE340_EmbeddedSystems/Spring2022/Labs/lab2/step1/interrupt_arm/lab2_interrupt_arm/design_simple_wrapper.xsa}\
-proc {ps7_cortexa9_0} -os {standalone} -fsbl-target {psu_cortexa53_0} -out {/home/nbellas/workspace2/lab2_interrupt_arm}

platform write
platform generate -domains 
platform active {lab2_interrupt_arm_platform}
platform generate
