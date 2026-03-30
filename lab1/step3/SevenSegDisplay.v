`timescale 1ns / 1ps

module SevenSegDisplay (
    input clk,
    input rst,
    input [7:0] DataIn,
    output reg an,
    output reg a,b,c,d,e,f,g
);

    reg [4:0] counter; // 5 bits counter (32 different values)
    reg [3:0] current_hex;

    // every 10ns the counter changes, therefore its period is 320ns = 0.32us
    always @(posedge clk) begin
        if (rst) 
            counter <= 5'b0;
        else 
            counter <= counter + 1; // Loops to 0 after 31
    end
  
    always @(counter or DataIn) begin
        if (counter[4] == 1'b0) begin // For the first 16 cycles
            an = 1'b0; // Enable the left part
            current_hex = DataIn[3:0];
        end else begin // For the last 16 cycles
            an = 1'b1; // Enable the right part
            current_hex = DataIn[7:4];
        end

          case (current_hex)
                4'b0000: {a,b,c,d,e,f,g} = 7'b1111110; // 0
                4'b0001: {a,b,c,d,e,f,g} = 7'b0110000; // 1
                4'b0010: {a,b,c,d,e,f,g} = 7'b1101101; // 2
                4'b0011: {a,b,c,d,e,f,g} = 7'b1111001; // 3
                4'b0100: {a,b,c,d,e,f,g} = 7'b0110011; // 4
                4'b0101: {a,b,c,d,e,f,g} = 7'b1011011; // 5 
                4'b0110: {a,b,c,d,e,f,g} = 7'b1011111; // 6
                4'b0111: {a,b,c,d,e,f,g} = 7'b1110000; // 7
                4'b1000: {a,b,c,d,e,f,g} = 7'b1111111; // 8
                4'b1001: {a,b,c,d,e,f,g} = 7'b1110011; // 9 
                4'b1010: {a,b,c,d,e,f,g} = 7'b1111101; // A
                4'b1011: {a,b,c,d,e,f,g} = 7'b0011111; // b
                4'b1100: {a,b,c,d,e,f,g} = 7'b1001110; // C
                4'b1101: {a,b,c,d,e,f,g} = 7'b0111101; // d
                4'b1110: {a,b,c,d,e,f,g} = 7'b1001111; // E
                4'b1111: {a,b,c,d,e,f,g} = 7'b1000111; // F
//                default: {a,b,c,d,e,f,g} = 7'b0000000; // All off
          endcase
    end

endmodule