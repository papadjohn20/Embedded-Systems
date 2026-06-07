/********************************************************************************
 * The Host code running in the Arm CPU. Make sure to study and understand the OpenCL API code
 *
 ********************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <CL/opencl.h>
#include <CL/cl_ext.h>

const short GAP_i = -1;
const short GAP_d = -1;
const short MATCH = 2;
const short MISS_MATCH = -1;
const short CENTER = 0;
const short NORTH = 1;
const short NORTH_WEST = 2;
const short WEST = 3;

// #define N 32
// #define M 40

#define N 32
#define M 65536

/***************************************************************************************
  * This is the golden code which runs in the CPU (and is the same code that you developed for x86 / Arm)
  * It will be used to verify the correct functionality of the HW implementation.
  * Its usefulness is mainly when you perform software emulation (sw_emu).
  ***************************************************************************************/
void compute_matrices_sw(
	char *string1, char *string2,
	int *max_index, int *similarity_matrix, short *direction_matrix)
{
	int i, j, index;
    int north, west, northwest;
    int val;
    short dir;
    int match_score;
    int max_val = -1;
    *max_index = 0;

    for (j = 1; j <= M; j++) {
        int row_offset = j * (N + 1);
        char string2_temp = string2[j-1];

        for (i = 1; i <= N; i++) {
            index = row_offset + i;

            north = similarity_matrix[index - N - 1];
            west =  similarity_matrix[index - 1];
            northwest = similarity_matrix[index - N - 2];

            val = 0;
            dir = CENTER;

            match_score = (string1[i-1] == string2_temp) ? MATCH : MISS_MATCH;
            int test_nw = northwest + match_score;
            if (test_nw > val) {
                val = test_nw;
                dir = NORTH_WEST;
            }

            int test_n = north + GAP_i;
            if (test_n > val) {
                val = test_n;
                dir = NORTH;
            }

            int test_w = west + GAP_d;
            if (test_w > val) {
                val = test_w;
                dir = WEST;
            }

            similarity_matrix[index] = val;
            direction_matrix[index] = dir;

            if (val > max_val) {
                max_val = val;
                *max_index = index;
            }
        }
    }

}

/*
 Given an event, this function returns the kernel execution time in ms
 */
double getTimeDifference(cl_event event) {
	cl_ulong time_start = 0;
	cl_ulong time_end = 0;
	double total_time = 0.0f;

	clGetEventProfilingInfo(event,
	CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start,
	NULL);
	clGetEventProfilingInfo(event,
	CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end,
	NULL);
	total_time = time_end - time_start;
	return total_time / 1000000.0; // To convert nanoseconds to milliseconds
}


/*
 return a random number between 0 and limit inclusive.
 */
int rand_lim(int limit) {

	int divisor = RAND_MAX / (limit + 1);
	int retval;

	do {
		retval = rand() / divisor;
	} while (retval > limit);

	return retval;
}

/*
 Fill the string with random values
 */
void fillRandom(char* string, int dimension) {
	//fill the string with random letters..
	static const char possibleLetters[] = "ATCG";

	string[0] = '-';

	int i;
	for (i = 0; i < dimension; i++) {
		int randomNum = rand_lim(3);
		string[i] = possibleLetters[randomNum];
	}

}

int load_file_to_memory(const char *filename, char **result) {
	size_t size = 0;
	FILE *f = fopen(filename, "rb");
	if (f == NULL) {
		*result = NULL;
		return -1; // -1 means file opening fail
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	*result = (char *) malloc(size + 1);
	if (size != fread(*result, sizeof(char), size, f)) {
		free(*result);
		return -2; // -2 means file reading fail
	}
	fclose(f);
	(*result)[size] = 0;
	return size;
}

/*******************************************************************************
 * Host program running on the Arm CPU.
 * The code is written using the OpenCL API.
 * We have provided multiple comments for you to understand where each thing
 *******************************************************************************/
int main(int argc, char** argv) {
	printf("starting HOST code \n");
	fflush(stdout);
	int err; // error code returned from api calls

	if (argc < 2) {
		printf("%s <input xclbin file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int matrix_size = (N + 1) * (M + 1);
    size_t similarity_bytes = matrix_size * sizeof(int);
    size_t directional_bytes = matrix_size * sizeof(short);

	char *query = (char*) malloc(sizeof(char) * N);
	char *database = (char*) malloc(sizeof(char) * M);

	int *similarity_matrix = (int*) malloc(similarity_bytes);
	short *direction_matrix = (short*) malloc(directional_bytes);
	int *max_index = (int *) malloc(sizeof(int));

	printf("array defined! \n");
    fflush(stdout);

	fillRandom(query, N);
	fillRandom(database, M);

	memset(similarity_matrix, 0, similarity_bytes);
	memset(direction_matrix, 0, directional_bytes);

	cl_platform_id platform_id;
	cl_device_id device_id; 
	cl_context context;         
	cl_command_queue commands;
	cl_program program;          
	cl_kernel kernel;                 

	char cl_platform_vendor[1001];
	char cl_platform_name[1001];

	cl_mem input_query;
	cl_mem input_database;
	cl_mem output_similarity_matrix;
	cl_mem output_direction_matrix;
	cl_mem output_max_index;

/**********************************************
 * Xilinx OpenCL Initialization
 *
 * We must follow specific steps to get the necessary
 * information and handlers, in order to be able
 * to use the available accelerator device (FPGA).
 * After every step, we always check for any errors
 * that might have occured. In case of error, the
 * program aborts and exits immediately.
 * *********************************************/

  /**************************************************
	* Step 1:
   * Get available OpenCL platforms and devices.
   * In our case, it is a Xilinx FPGA device.
   * If the underlying platform has other accelerators
   * available, we could use them too (e.g. GPU, CPU).
	**************************************************/
	printf("GET platform \n");
	err = clGetPlatformIDs(1, &platform_id, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to find an OpenCL platform!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("GET platform vendor \n");
	err = clGetPlatformInfo(platform_id, CL_PLATFORM_VENDOR, 1000,
			(void *) cl_platform_vendor, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: clGetPlatformInfo(CL_PLATFORM_VENDOR) failed!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("CL_PLATFORM_VENDOR %s\n", cl_platform_vendor);
	printf("GET platform name \n");
	err = clGetPlatformInfo(platform_id, CL_PLATFORM_NAME, 1000,
			(void *) cl_platform_name, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: clGetPlatformInfo(CL_PLATFORM_NAME) failed!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("CL_PLATFORM_NAME %s\n", cl_platform_name);

	// Connect to a compute device
	//
	int fpga = 1;

	printf("get device FPGA is %d  \n", fpga);
	err = clGetDeviceIDs(platform_id,
			fpga ? CL_DEVICE_TYPE_ACCELERATOR : CL_DEVICE_TYPE_CPU, 1,
			&device_id, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to create a device group!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	/*********************************************
	 * Step 2 : Create Context
	 *********************************************/
	printf("create context \n");
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
	if (!context) {
		printf("Error: Failed to create a compute context!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

   /*********************************************
	 * Step 3 : Create Command Queue
	 *********************************************/
	printf("create queue \n");
	commands = clCreateCommandQueue(context, device_id,
	CL_QUEUE_PROFILING_ENABLE, &err);
	if (!commands) {
		printf("Error: Failed to create a command commands!\n");
		printf("Error: code %i\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	cl_int binary_status;

	/*********************************************
	 * Step 4 : Load Hardware Binary File (*.xclbin) from disk
	 **********************************************/
	unsigned char *kernelbinary;
	char *xclbin = argv[1];
	printf("loading %s\n", xclbin);
	int n_i = load_file_to_memory(xclbin, (char **) &kernelbinary);
	if (n_i < 0) {
		printf("failed to load kernel from xclbin: %s\n", xclbin);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

  /********************************************************
	* Step 5 : Create program using the loaded hardware binary file
	********************************************************/
	size_t n = n_i;
	// Create the compute program from offline
	printf("create program with binary \n");
	program = clCreateProgramWithBinary(context, 1, &device_id, &n,
			(const unsigned char **) &kernelbinary, &binary_status, &err);
	free(kernelbinary);

	if ((!program) || (err != CL_SUCCESS)) {
		printf("Error: Failed to create compute program from binary %d!\n",
				err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	/**************************************************************
	 * Step 6 for 1 Compute Unit: Create Kernels - the actual handler of the kernel that
    * we will be using. We first create a program, and then
    * obtain the kernel handler from the program.
	 **************************************************************/
	printf("build program \n");
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS) {
		size_t len;
		char buffer[2048];

		printf("Error: Failed to build program executable!\n");
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG,
				sizeof(buffer), buffer, &len);
		printf("%s\n", buffer);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	// Create the compute kernel in the program we wish to run
	//
	printf("create kernel \n");
	kernel = clCreateKernel(program, "compute_matrices", &err);
	if (!kernel || err != CL_SUCCESS) {
		printf("Error: Failed to create compute kernel!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

    /**************************************************************-
    * Step 7 : Create buffers.
    * We do not need to allocate separate memory space (malloc), because
    * on a MPSoC system (e.g. ZCU102 board), we map the memory space that is
    * allocated at clCreateBuffer, to a usable memory space for our
    * host application. We also do not need to use free for any reason.
    * See Xilinx UG1393 for detailed information.
    **************************************************************/
	input_query = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(char) * N,
	NULL, NULL);
	input_database = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(char) * M,
	NULL, NULL);
	output_similarity_matrix = clCreateBuffer(context, CL_MEM_READ_WRITE, similarity_bytes,
	NULL, NULL);
	output_direction_matrix = clCreateBuffer(context, CL_MEM_READ_WRITE, directional_bytes,
	NULL, NULL);
	output_max_index = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int),
	NULL, NULL);

	if (!input_query || !input_database || !output_similarity_matrix
			|| !output_direction_matrix || !output_max_index) {
		printf("Error: Failed to allocate device memory!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

   /**************************************************************
    * Step 8 : Write the Input Data to the Write Buffers of the device memory
    **************************************************************/
	err = clEnqueueWriteBuffer(commands, input_query, CL_TRUE, 0,
			sizeof(char) * N, query, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to write to source array a!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	err = clEnqueueWriteBuffer(commands, input_database, CL_TRUE, 0,
			sizeof(char) * M, database, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to write to source array a!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	err = clEnqueueWriteBuffer(commands, output_similarity_matrix, CL_TRUE, 0, similarity_bytes,
			similarity_matrix, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to write to source array a!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	err = clEnqueueWriteBuffer(commands, output_direction_matrix, CL_TRUE, 0, directional_bytes,
			direction_matrix, 0, NULL, NULL);
		if (err != CL_SUCCESS) {
			printf("Error: Failed to write to source array a!\n");
			printf("Test failed\n");
			return EXIT_FAILURE;
		}

	/**************************************************************
	 * Step 9: Set the arguments to our compute kernel
	 **************************************************************/
	err = 0;
	printf("set arg 0 \n");
	err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_query);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to set kernel arguments 0! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("set arg 1 \n");
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &input_database);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to set kernel arguments 1! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("set arg 2 \n");
	err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &output_max_index);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to set kernel arguments 2! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("set arg 3 \n");
	err |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &output_similarity_matrix);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to set kernel arguments 3! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("set arg 4 \n");
	err |= clSetKernelArg(kernel, 4, sizeof(cl_mem), &output_direction_matrix);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to set kernel arguments 4! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}


	/**************************************************************
	 * Step 10: Place the Kernel in the Queue for Execution
	 **************************************************************/
	cl_event enqueue_kernel;
	printf("LAUNCH task \n");
	err = clEnqueueTask(commands, kernel, 0, NULL, &enqueue_kernel);

	if (err) {
		printf("Error: Failed to execute kernel! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	clWaitForEvents(1, &enqueue_kernel);

	/**************************************************************
	 * Step 11: Read back the results from the device to verify the output
	 **************************************************************/
	cl_event readMax, readSimilarity, readDirections;
	err = clEnqueueReadBuffer(commands, output_similarity_matrix, CL_TRUE, 0,
			similarity_bytes, similarity_matrix, 0, NULL, &readSimilarity);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to read array! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	err = clEnqueueReadBuffer(commands, output_direction_matrix, CL_TRUE, 0,
			directional_bytes, direction_matrix, 0, NULL, &readDirections);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to read array! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	err = clEnqueueReadBuffer(commands, output_max_index, CL_TRUE, 0,
			sizeof(int), max_index, 0, NULL, &readMax);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to read array! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	clWaitForEvents(1, &readSimilarity);
	clWaitForEvents(1, &readDirections);
	clWaitForEvents(1, &readMax);

	double executionTime = getTimeDifference(enqueue_kernel);


    /**************************************************************
    * Run the same algorithm in the Host Unit and compare for verification
    **************************************************************/
	int matrix_size_sw = (N+1) * (M+1);
	int *similarity_matrix_sw = (int*) malloc(sizeof( int) * matrix_size_sw);
	short *direction_matrix_sw = (short*) malloc(sizeof( short) * matrix_size_sw);
	int *max_index_sw = (int*) malloc(sizeof( int));

	for(cl_uint i = 0; i < matrix_size_sw; i++){
		similarity_matrix_sw[i] = 0;
		direction_matrix_sw[i] = 0;
	}
	compute_matrices_sw(query, database, max_index_sw, similarity_matrix_sw, direction_matrix_sw);

	printf("both ended serial opt\n");
	printf("execution time is %lf ms \n", executionTime);

	int errors = 0;
	for (int row = 0; row <= M; row++) {
		for (int col = 0; col <= N; col++) {
			int i = row * (N + 1) + col;

			if (direction_matrix_sw[i] != direction_matrix[i]) {
				errors++;
				if (errors < 5) {
					printf("Direction Mismatch at row %d, col %d (index %d): HW=%d, SW=%d\n",
						row, col, i, direction_matrix[i], direction_matrix_sw[i]);
				}
			}
		}
	}

    if (*max_index != *max_index_sw) {
        printf("Error: max_index mismatch! HW: %d, SW: %d\n", *max_index, *max_index_sw);
        errors++;
    } else {
        int max_idx_val = *max_index;
        int row = max_idx_val / (N + 1);
        int col = max_idx_val % (N + 1);
        printf("\nCORRECT MAX INDEX at row %d, col %d (index %d)\n", row, col, max_idx_val);
    }

	if (errors == 0) {
        printf("computation ended!- RESULTS CORRECT \n");
    } else {
        printf("TEST FAILED! Total errors: %d\n", errors);
    }

	/**************************************************************
	 * Clean up everything and, then, shutdown
	 **************************************************************/

	clReleaseMemObject(input_database);
	clReleaseMemObject(input_query);
	clReleaseMemObject(output_direction_matrix);
	clReleaseMemObject(output_max_index);
	clReleaseMemObject(output_similarity_matrix);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);


    free(query);
    free(database);
    free(similarity_matrix);
	free(direction_matrix);
	free(max_index);
	free(similarity_matrix_sw);
	free(direction_matrix_sw);
	free(max_index_sw);

	// return EXIT_SUCCESS;
	return (errors == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
