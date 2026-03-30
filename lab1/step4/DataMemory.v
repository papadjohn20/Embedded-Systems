`timescale 1ns / 1ps

module DataMemory (
    input clk,
    input rst,
    input btn_pulse,
    output reg [31:0] reg_A, 
    output reg [31:0] reg_B
);

    parameter NUM = 10;
    reg [3:0] counter; // counter for 10 lines

    // Counter logic
    always @(posedge clk) begin
        if (rst) begin
            counter <= 0;
        end 
        else if (btn_pulse) begin
            if (counter == NUM - 1) // when it reaches line 10 reset counter
                counter <= 0;
            else
                counter <= counter + 1;
        end
    end

    // Values from fp_InOut.hex 
    always @(counter) begin
        case(counter)
            4'd0: begin reg_A = 32'h3f800000; reg_B = 32'h40000000; end
            4'd1: begin reg_A = 32'hbf800000; reg_B = 32'h3f800000; end
            4'd2: begin reg_A = 32'hc2de8000; reg_B = 32'h45155e00; end
            4'd3: begin reg_A = 32'h6b64b235; reg_B = 32'h6ac49214; end
            4'd4: begin reg_A = 32'h2ac49214; reg_B = 32'h6ac49214; end
            4'd5: begin reg_A = 32'hbfc66666; reg_B = 32'h3fc7ae14; end
            4'd6: begin reg_A = 32'hc565ee8b; reg_B = 32'h4565ee8a; end
            4'd7: begin reg_A = 32'h447a4efa; reg_B = 32'hc47a1ccd; end
            4'd8: begin reg_A = 32'h00000000; reg_B = 32'h00000000; end
            4'd9: begin reg_A = 32'h38108900; reg_B = 32'hbb908900; end
            default: begin reg_A = 32'h0; reg_B = 32'h0; end
        endcase
    end

endmodule