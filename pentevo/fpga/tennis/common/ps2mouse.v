module ps2mouse(
 input wire glb_clk,
 input wire reset,
 input wire slowtick,
 input wire sw_sens,

 output reg data_ready,
 output reg [23:0] data_buff,

 inout wire ps2_clk_io,
 inout wire ps2_data_io
);

 localparam STM_RESET    = 3'd0;
 localparam STM_SETUP00  = 3'd1;
 localparam STM_SETUP01  = 3'd2;
 localparam STM_SETUP10  = 3'd3;
 localparam STM_SETUP11  = 3'd4;
 localparam STM_MAIN     = 3'd5;

 reg [1:0] prev_swsens; initial prev_swsens = 1'd0;
 reg [1:0] sens;        initial sens = 2'd2;
 reg [2:0] state;       initial state = STM_RESET;
 reg [5:0] slow_cnt;
 reg [1:0] cmdstep;     initial cmdstep = 2'd0;
 reg [1:0] data_cnt;    initial data_cnt = 2'd0;
 reg [7:0] data_out;
 reg ibf_clr, obf_set, err_clr;

 wire [7:0] data_in;
 wire ibf, frame_err, parity_err;

 ps2 ps2lowlevel( .clk_i(glb_clk),
                  .rst_i(~reset),
                  .data_o(data_in),
                  .data_i(data_out),
                  .ibf_clr_i(ibf_clr),
                  .obf_set_i(obf_set),
                  .ibf_o(ibf),
                  .frame_err_o(frame_err),
                  .parity_err_o(parity_err),
                  .err_clr_i(err_clr),
                  .ps2_clk_io(ps2_clk_io),
                  .ps2_data_io(ps2_data_io)
                );


 always @(negedge glb_clk or posedge reset)
 begin
  if (reset)
  begin
   state <= STM_RESET;
   data_cnt <= 2'd0;
   data_ready <= 1'd0;
   prev_swsens <= 1'd0;
  end
  else
  begin
   case (state)
    STM_RESET:
      begin
       state <= STM_SETUP00;
       ibf_clr <= 1'd0;
       obf_set <= 1'd0;
      end
    STM_SETUP00:
      begin
       data_out <= 8'hff;
       data_cnt <= 2'd0;
       state <= STM_SETUP01;
       ibf_clr <= 1'd0;
       obf_set <= 1'd1;
      end
    STM_SETUP01:
      begin
       if (data_cnt==2'd3)
       begin
        if (data_buff==24'h00aafa)
        begin
         state <= STM_SETUP10;
         cmdstep <= 2'd0;
        end
        else
         state <= STM_RESET;
        ibf_clr <= 1'd0;
       end
       else if (ibf)
       begin
        if (data_cnt[1])
         data_buff[23:16] <= data_in;
        else if (data_cnt[0])
         data_buff[15:8] <= data_in;
        else
         data_buff[7:0] <= data_in;
        data_cnt <= data_cnt+2'd1;
        ibf_clr <= 1'd1;
       end
       else if (slow_cnt==6'd63)
       begin
        state <= STM_RESET;
        ibf_clr <= 1'd0;
       end
       else
        ibf_clr <= 1'd0;
       obf_set <= 1'd0;
      end
    STM_SETUP10:
      begin
       if (cmdstep==2'd2)
        data_out <= { 6'd0, sens };     // data_out <= 8'h02;
       else if (cmdstep==2'd1)
        data_out <= 8'he8;
       else
        data_out <= 8'hf4;
       cmdstep <= cmdstep + 2'd1;
       data_cnt <= 2'd0;
       state <= STM_SETUP11;
       ibf_clr <= 1'd0;
       obf_set <= 1'd1;
      end
    STM_SETUP11:
      begin
       if (data_cnt[0])
       begin
        if (data_buff[7:0]==8'hfa)
        begin
         if (cmdstep==2'd3)
         begin
          data_cnt <= 2'd0;
          state <= STM_MAIN;
         end
         else
          state <= STM_SETUP10;
        end
        else
         state <= STM_RESET;
        ibf_clr <= 1'd0;
       end
       else if (ibf)
       begin
        data_buff[7:0] <= data_in;
        data_cnt[0] <= 1'd1;
        ibf_clr <= 1'd1;
       end
       else if (slow_cnt==6'd63)
       begin
        state <= STM_RESET;
        ibf_clr <= 1'd0;
       end
       else
        ibf_clr <= 1'd0;
       obf_set <= 1'd0;
      end
    STM_MAIN:
      begin
       if (data_cnt==2'd3)
       begin
        ibf_clr <= 1'd0;
        data_ready <= 1'd1;
        data_cnt <= 2'd0;
       end
       else if (ibf)
       begin
        if (data_cnt[1])
         data_buff[23:16] <= data_in;
        else if (data_cnt[0])
         data_buff[15:8] <= data_in;
        else
         data_buff[7:0] <= data_in;
        data_cnt <= data_cnt+2'd1;
        ibf_clr <= 1'd1;
        data_ready <= 1'd0;
       end
       else if ( ~sw_sens & prev_swsens & (data_cnt==2'd0) )
       begin
        sens <= sens + 2'd1;
        data_cnt <= 2'd0;
        cmdstep <= 2'd1;
        state <= STM_SETUP10;
        ibf_clr <= 1'd0;
        data_ready <= 1'd0;
       end
       else
       begin
        ibf_clr <= 1'd0;
        data_ready <= 1'd0;
       end
       obf_set <= 1'd0;
      end
   endcase
   err_clr <= frame_err | parity_err;
   prev_swsens <= sw_sens;
  end
 end

 always@(posedge slowtick or posedge obf_set)
 begin
  if (obf_set)
   slow_cnt <= 6'd0;
  else
   slow_cnt <= slow_cnt + 6'd1;
 end

endmodule
