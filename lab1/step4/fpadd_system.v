/*
 -----------------------------------------------------------------------------
 -- File           : fpadd_system.v
 -----------------------------------------------------------------------------
 */ 
 module fpadd_system (input clk,
                      input rst, 
                      // input noisy_level,
                      input btn,
                      output [7:0] leds, 
                      output an0, output a0, output b0, output c0, output d0, output e0, output f0, output g0,
                      output an1, output a1, output b1, output c1, output d1, output e1, output f1, output g1);

   wire [31:0] fp_out;
   wire debounced_btn;

   wire [31:0] A;
   wire [31:0] B;

   // Instantiate Debouncer
   Debouncer debouncer_dut (.clk(clk), .async_btn(btn), .btn_out(debounced_btn));

   // Instantiate DataMemory
   DataMemory dataMemory_dut (.clk(clk), .rst(rst), .btn_pulse(debounced_btn), .reg_A(A), .reg_B(B));
   
   // Instantiate the FP adder 
	fpadd_single fpadd_single_dut(.clk(clk), .reset(rst), .reg_A(A), .reg_B(B), .out(fp_out));
    
   assign leds = fp_out[7:0];
   
   // Instantiate the 7segment display output 0 
   SevenSegDisplay SSD0 (
      .clk(clk),
      .rst(rst), 
      .DataIn(fp_out[23:16]), 
      .an(an0),
      .a(a0),.b(b0),.c(c0),.d(d0),.e(e0),.f(f0),.g(g0)
   );
   
   // Instantiate the 7segment display output 1
   SevenSegDisplay SSD1 (
      .clk(clk),
      .rst(rst), 
      .DataIn(fp_out[31:24]), 
      .an(an1),
      .a(a1),.b(b1),.c(c1),.d(d1),.e(e1),.f(f1),.g(g1)
   );

endmodule
