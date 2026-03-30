`timescale 1ns / 1ps
`define CYCLE 10

module fpadd_system_tb();
    reg clk;
    reg rst;
    reg btn;
    wire [7:0] leds;
    wire an0, a0, b0, c0, d0, e0, f0, g0;
    wire an1, a1, b1, c1, d1, e1, f1, g1;

    // Instantiate top module
    fpadd_system fpadd_system_uut (
        .clk(clk),
        .rst(rst),
        .btn(btn),
        .leds(leds),
        .an0(an0), .a0(a0), .b0(b0), .c0(c0), .d0(d0), .e0(e0), .f0(f0), .g0(g0),
        .an1(an1), .a1(a1), .b1(b1), .c1(c1), .d1(d1), .e1(e1), .f1(f1), .g1(g1)
    );



    initial begin
        clk = 0;
        rst = 1;
        btn = 0;
        #(`CYCLE * 5);
        rst = 0;

        #(`CYCLE * 10);

        btn = 1;
        #(`CYCLE * 10);
        btn = 0;

        #(`CYCLE * 6);

        btn = 1;
        #(`CYCLE * 3);
        btn = 0;
        
        #(`CYCLE * 6);
        
        btn = 1;
        #(`CYCLE * 7);
        btn = 0;

        #(1000); 
        $finish;
    end

	always
		begin
			#(`CYCLE/2) clk=~clk;
		end

endmodule