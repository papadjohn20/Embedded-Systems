#include "xparameters.h"
#include "xgpio.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xil_io.h"

#define FP_ADDER_BASE 0x41220000 

// GPIO & Interrupt Definitions (from lab2_interrupt.c)
#define INTC_DEVICE_ID          XPAR_PS7_SCUGIC_0_DEVICE_ID
#define BTNS_DEVICE_ID          XPAR_GPIO_0_DEVICE_ID
#define INTC_GPIO_INTERRUPT_ID  XPAR_FABRIC_BTNS_5BIT_IP2INTC_IRPT_INTR
#define BTN_INT                 XGPIO_IR_CH1_MASK

XGpio BTNInst;
XScuGic INTCInst;
static int btn_value;
static int current_test = 0;

u32 test_cases[10][3] = {
    {0x3f800000, 0x40000000, 0x40400000},
    {0xbf800000, 0x3f800000, 0x00000000},
    {0xc2de8000, 0x45155e00, 0x450e6a00},
    {0x6b64b235, 0x6ac49214, 0x6ba37d9f},
    {0x2ac49214, 0x6ac49214, 0x6ac49214},
    {0xbfc66666, 0x3fc7ae14, 0x3c23d700},
    {0xc565ee8b, 0x4565ee8a, 0xb9800000},
    {0x447a4efa, 0xc47a1ccd, 0x3f48b400},
    {0x00000000, 0x00000000, 0x00000000},
    {0x38108900, 0xbb908900, 0xbb8f67ee}
};

// Prototypes
static void BTN_Intr_Handler(void *baseaddr_p);
static void execute_fp_addition();
static int InterruptSystemSetup(XScuGic *XScuGicInstancePtr);
static int IntcInitFunction(u16 DeviceId, XGpio *GpioInstancePtr);

void BTN_Intr_Handler(void *InstancePtr) {
    XGpio_InterruptDisable(&BTNInst, BTN_INT);
    if ((XGpio_InterruptGetStatus(&BTNInst) & BTN_INT) != BTN_INT) {
        return;
    }

    btn_value = XGpio_DiscreteRead(&BTNInst, 1);

    if (btn_value == 1) { // Center Button (Reset)
        current_test = 0;
        xil_printf("\n\r--- Reset to Test Case 0 ---\n\r");
        execute_fp_addition();
    } 
    else if (btn_value == 8) { // Right Button (Next)
        current_test = (current_test + 1) % 10;
        xil_printf("\n\r--- Moved to the next test ---\n\r");
    	execute_fp_addition();
    }
    else if (btn_value == 4) { // Left Button (Prev)
    	if (current_test == 0) {
    		current_test = 9;
        } else {
        	current_test = current_test - 1;
        }
        xil_printf("\n\r--- Moved to the previous test ---\n\r");
        execute_fp_addition();
    }

    (void)XGpio_InterruptClear(&BTNInst, BTN_INT);
    XGpio_InterruptEnable(&BTNInst, BTN_INT);
}


void execute_fp_addition() {
    Xil_Out32(FP_ADDER_BASE,     test_cases[current_test][0]); // slv_reg0
    Xil_Out32(FP_ADDER_BASE + 4, test_cases[current_test][1]); // slv_reg1

    u32 result = Xil_In32(FP_ADDER_BASE + 8); // slv_reg2

    xil_printf("[%d] A:%08x + B:%08x | Result:%08x | Expected:%08x\n\r",
               current_test, test_cases[current_test][0],
               test_cases[current_test][1], result, test_cases[current_test][2]);
}
int main (void) {
    xil_printf("FP Adder Application Started. Press: \n\r"
    		"--- Right Button for Next Test.\n\r"
    		"--- Center for Reset.\n\r"
    		"--- Left for Previous Test.\n\r");
    int status;
    //----------------------------------------------------
    // INITIALIZE THE PERIPHERALS & SET DIRECTIONS OF GPIO
    //----------------------------------------------------
    // Initialize Push Buttons
    status = XGpio_Initialize(&BTNInst, BTNS_DEVICE_ID);
    if(status != XST_SUCCESS) return XST_FAILURE;
    // Set all buttons direction to inputs
    XGpio_SetDataDirection(&BTNInst, 1, 0xFF);

    // Initialize interrupt controller
    status = IntcInitFunction(INTC_DEVICE_ID, &BTNInst);
    if(status != XST_SUCCESS) return XST_FAILURE;

    while(1); // endless loop

    return 0;
}

//----------------------------------------------------
// INITIAL SETUP FUNCTIONS (lab2_interrupt.c)
//----------------------------------------------------

int InterruptSystemSetup(XScuGic *XScuGicInstancePtr)
{
	// Enable interrupt
	XGpio_InterruptEnable(&BTNInst, BTN_INT);
	XGpio_InterruptGlobalEnable(&BTNInst);

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			 	 	 	 	 	 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
			 	 	 	 	 	 XScuGicInstancePtr);
	Xil_ExceptionEnable();


	return XST_SUCCESS;

}

int IntcInitFunction(u16 DeviceId, XGpio *GpioInstancePtr) {
	XScuGic_Config *IntcConfig;
	int status;

	// Interrupt controller initialization
	IntcConfig = XScuGic_LookupConfig(DeviceId);
	status = XScuGic_CfgInitialize(&INTCInst, IntcConfig, IntcConfig->CpuBaseAddress);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// Call to interrupt setup
	status = InterruptSystemSetup(&INTCInst);
	if(status != XST_SUCCESS) return XST_FAILURE;
	
	// Connect GPIO interrupt to handler
	status = XScuGic_Connect(&INTCInst,
					  	  	 INTC_GPIO_INTERRUPT_ID,
					  	  	 (Xil_ExceptionHandler)BTN_Intr_Handler,
					  	  	 (void *)GpioInstancePtr);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// Enable GPIO interrupts interrupt
	XGpio_InterruptEnable(GpioInstancePtr, 1);
	XGpio_InterruptGlobalEnable(GpioInstancePtr);

	// Enable GPIO and timer interrupts in the controller
	XScuGic_Enable(&INTCInst, INTC_GPIO_INTERRUPT_ID);
	
	return XST_SUCCESS;
}
