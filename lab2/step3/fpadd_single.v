`timescale 1ns / 1ps
module fpadd_single (input clk,
                     input reset,
                     input [31:0]reg_A, 
                     input [31:0]reg_B,  
		    		 output reg[31:0] out);
				     	
	// Register the two inputs, and use A and B in the combinational logic. 
	reg [31:0] A, B;
	reg [31:0] result;
	reg [23:0] mantissa_A, mantissa_B; // 24 bits to include the hidden "1"
	integer exp_diff; // difference of the exponents of A and B
	reg [7:0] res_exp;
	reg res_sign;
	reg res_sign_pipe;
	reg [24:0] res_mant; //  25 bits to catch the carry out
	reg [24:0] res_mant_pipe, res_mant_partD; 
	//reg [23:0] mantissa_A_pipe, mantissa_B_pipe; 
	reg [7:0] res_exp_pipe, res_exp_partD;

	reg [4:0] leading_zeros;
	
	always @ (posedge clk)
		begin
			if (reset == 1'b1) begin
				A <= 32'b0;
                B <= 32'b0;
                out <= 32'b0;
				res_mant_pipe <= 25'b0;
                res_exp_pipe <= 8'b0;
                res_sign_pipe <= 1'b0;
			end else begin
                A <= reg_A;
                B <= reg_B;
                out <= result;
				res_mant_pipe <= res_mant;
                res_exp_pipe <= res_exp;
                res_sign_pipe <= res_sign;
			end
		end
		
	//Combinational Logic to (a) compare and adjust the exponents, 
	//                       (b) shift appropriately the mantissa if necessary, 
	//                       (c) add the two mantissas, and
	//                       (d) perform post-normalization. 
	//                           Make sure to check explicitly for zero output. 


	always @ (*) begin

		mantissa_A = (A[30:0] == 0) ? 24'b0 : {1'b1, A[22:0]};
        mantissa_B = (B[30:0] == 0) ? 24'b0 : {1'b1, B[22:0]};

			//***************************
			// 1ST STAGE OF THE PIPELINE
			//***************************
			
			// -------PART A and B-------
		    if (A[30:23] > B[30:23]) begin
				exp_diff = A[30:23] - B[30:23];
				mantissa_B = mantissa_B >> exp_diff;
				res_exp = A[30:23]; 
		    end
			else begin
				exp_diff = B[30:23] - A[30:23];
				mantissa_A = mantissa_A >> exp_diff;
				res_exp = B[30:23];
		    end
			 

			// -------PART C-------
			if (A[31] == B[31]) begin
				res_mant = mantissa_A + mantissa_B;
				res_sign = A[31];
			end else if (mantissa_A >= mantissa_B) begin
				res_mant = mantissa_A - mantissa_B;
				res_sign = A[31];
			end else begin
				res_mant = mantissa_B - mantissa_A;
				res_sign = B[31];
			end
			
			//***************************
			// 2ND STAGE OF THE PIPELINE
			//***************************
			
			res_exp_partD = res_exp_pipe;
			res_mant_partD = res_mant_pipe;
			// -------PART D-------
			if (res_mant_partD == 25'b0) begin // checking explicitly for zero output
				result = 32'b0;
			end else begin
				if (res_mant_partD[24]) begin // 10. ...  or  11. ... 
					res_mant_partD = res_mant_partD >> 1;
					res_exp_partD = res_exp_partD + 1;
				end else begin
						if      (res_mant_partD[23]) leading_zeros = 5'd0;
						else if (res_mant_partD[22]) leading_zeros = 5'd1;
						else if (res_mant_partD[21]) leading_zeros = 5'd2;
						else if (res_mant_partD[20]) leading_zeros = 5'd3;
						else if (res_mant_partD[19]) leading_zeros = 5'd4;
						else if (res_mant_partD[18]) leading_zeros = 5'd5;
						else if (res_mant_partD[17]) leading_zeros = 5'd6;
						else if (res_mant_partD[16]) leading_zeros = 5'd7;
						else if (res_mant_partD[15]) leading_zeros = 5'd8;
						else if (res_mant_partD[14]) leading_zeros = 5'd9;
						else if (res_mant_partD[13]) leading_zeros = 5'd10;
						else if (res_mant_partD[12]) leading_zeros = 5'd11;
						else if (res_mant_partD[11]) leading_zeros = 5'd12;
						else if (res_mant_partD[10]) leading_zeros = 5'd13;
						else if (res_mant_partD[9])  leading_zeros = 5'd14;
						else if (res_mant_partD[8])  leading_zeros = 5'd15;
						else if (res_mant_partD[7])  leading_zeros = 5'd16;
						else if (res_mant_partD[6])  leading_zeros = 5'd17;
						else if (res_mant_partD[5])  leading_zeros = 5'd18;
						else if (res_mant_partD[4])  leading_zeros = 5'd19;
						else if (res_mant_partD[3])  leading_zeros = 5'd20;
						else if (res_mant_partD[2])  leading_zeros = 5'd21;
						else if (res_mant_partD[1])  leading_zeros = 5'd22;
						else if (res_mant_partD[0])  leading_zeros = 5'd23;
						else                   leading_zeros = 5'd24;

						if (leading_zeros > res_exp_partD) leading_zeros = res_exp_partD;

						res_mant_partD = res_mant_partD << leading_zeros;
						res_exp_partD = res_exp_partD - leading_zeros;
					end
				
				result = {res_sign_pipe, res_exp_partD, res_mant_partD[22:0]};
			end
		end
endmodule
