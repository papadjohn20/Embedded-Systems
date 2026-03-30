module Debouncer (
    input clk,
    input async_btn, // asyncronous button signal 
    output reg  btn_out // fixed input signal  
);
    parameter COUNT_MAX = 10_000_000; //real 
    //parameter COUNT_MAX = 5; //simulation
    reg [$clog2(COUNT_MAX):0] count = 0;
    reg stable_state = 1'b0;
    reg prev_stable_state = 1'b0; 
    reg ff1, sync_btn;

    always @(posedge clk) begin
        ff1  <= async_btn;   // 1st flip flop
        sync_btn  <= ff1;         // 2nd flip flop
    end

    always @(posedge clk) begin
        if (sync_btn != stable_state) begin
            count <= count + 1;
            if (count >= COUNT_MAX) begin
                stable_state <= sync_btn;
                count <= 0;
            end
        end else begin
            count <= 0;
        end

        prev_stable_state <= stable_state;
    end

    always @(stable_state or prev_stable_state) begin
        btn_out = (stable_state) && (!prev_stable_state);
    end 
endmodule