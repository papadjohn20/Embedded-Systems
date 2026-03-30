# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct /home/nbellas/workspace_step3/lab2_fpadd_platform/lab2_fpadd_platform/platform.tcl
# 
# OR launch xsct and run below command.
# source /home/nbellas/workspace_step3/lab2_fpadd_platform/lab2_fpadd_platform/platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {lab2_fpadd_platform}\
-hw {/home/nbellas/courses/ECE340_EmbeddedSystems/Spring2022/Labs/lab2/step3/lab2_fpadd_arm/lab2_fpadd_wrapper.xsa}\
-proc {ps7_cortexa9_0} -os {standalone} -fsbl-target {psu_cortexa53_0} -out {/home/nbellas/workspace_step3/lab2_fpadd_platform}

platform write
platform generate -domains 
platform active {lab2_fpadd_platform}
platform generate
platform generate
