// (c) 2010 NedoPC
//
// fpga SPI slave device.

`include "tune.v"

// In:
//  SCS_n = 1 - reg number
//  SCS_n = 0 - data in
//
// Out:
//  SCS_n = 1 - status
//  SCS_n = 0 - data out

// Registers:
//
//  0x10 - ZX keyboard data
//  0x11 - ZX keyboard stop bit
//
//  0x20 - ZX mouse X coordinate register
//  0x21 - ZX mouse Y coordinate register
//  0x22 - ZX mouse Y coordinate register
//  0x23 - Kempston joystick register
//
//  0x30 - ZX reset register
//
//  0x40 - ZX all data for wait registers
//  0x41 - ZX Gluk address register
//  0x42 - ZX Kondratiev's rs232 address register
//
//  0x50 - ZX configuration register

module slavespi
(
  // System
  input  wire fclk,
  input  wire rst_n,

  // Slave SPI
  input  wire spics_n, // avr SPI interface
  output wire spidi,   // to AVR
  input  wire spido,   // from AVR
  input  wire spick,

  input  wire [ 7:0] status_in,   // to indicate wait port source

  // ZX Keyboard
  output wire [ 7:0] kbd_out,
  output reg  [ 2:0] kbd_out_sel,
  output wire        kbd_stb,

  // ZX Mouse
  output wire [ 7:0] mus_out,
  output wire        mus_xstb,
  output wire        mus_ystb,
  output wire        mus_btnstb,
  output wire        kj_stb,

    // Glucklock and Comport
  input  wire [ 7:0] wait_addr,

  // Configuration
  output wire [ 7:0] config0,

  input  wire [ 7:0] wait_write,
  output wire [ 7:0] wait_read,
  output wire        wait_end,

  output reg         genrst  // positive pulse causes Z80 reset
);

  // output data
  assign kbd_out     = {sdo, shift_in[7:1]};
  assign kbd_stb     = kbd_bit_stb && &kbd_out_cnt[2:0];
  assign kbd_out_sel = kbd_out_cnt[5:3];
  assign mus_out     = shift_in;
  assign mus_xstb    = sel_musxcr && scs_n_01;
  assign mus_ystb    = sel_musycr && scs_n_01;
  assign mus_btnstb  = sel_musbtn && scs_n_01;
  assign kj_stb      = sel_kj     && scs_n_01;
  assign wait_read   = wait_reg;
  assign wait_end    = sel_waitreg && scs_n_01;
  assign config0     = {cfg0_reg[7:6], int_wtp, cfg0_reg[4:0]};

  // re-sync SPI
  reg [2:0] spics_n_sync;
  reg [1:0] spido_sync;
  reg [2:0] spick_sync;

  always @(posedge fclk)
  begin
    spics_n_sync[2:0] <= {spics_n_sync[1:0], spics_n};
    spido_sync[1:0] <= {spido_sync[0], spido};
    spick_sync[2:0] <= {spick_sync[1:0], spick};
  end

  wire scs_n = spics_n_sync[1]; // scs_n - synchronized CS_N
  wire sdo   = spido_sync[1];
  wire scs_n_01 = !spics_n_sync[2] && spics_n_sync[1];
  wire scs_n_10 = spics_n_sync[2] && !spics_n_sync[1];
  wire sck_01 = !spick_sync[2] && spick_sync[1];

  reg [7:0] regnum;
  reg [7:0] shift_out;
  reg [7:0] data_in;

  // register number
  always @(posedge fclk)
  begin
    if (scs_n_01)
      regnum <= 8'd0;
    else if (scs_n && sck_01)
      regnum[7:0] <= {sdo, regnum[7:1]};
  end

  // send data to avr
  always @*
  begin
    if (sel_waitreg)
      data_in = wait_write;
    else if (sel_gluadr || sel_comadr)
      data_in = wait_addr;
    else data_in = 8'hFF;
  end

  always @(posedge fclk)
  begin
    if (scs_n_01 || scs_n_10)   // both edges
      shift_out <= scs_n ? status_in : data_in;
    else if (sck_01)
      shift_out[7:0] <= {1'b0, shift_out[7:1]};
  end

  assign spidi = shift_out[0];

  // reg number decoding
  wire sel_kbdreg = (regnum[7:4] == 4'h1) && !regnum[0]; // $10
  wire sel_kbdstb = (regnum[7:4] == 4'h1) &&  regnum[0]; // $11

  wire sel_musxcr = (regnum[7:4] == 4'h2) && (regnum[1:0] == 2'b00); // $20
  wire sel_musycr = (regnum[7:4] == 4'h2) && (regnum[1:0] == 2'b01); // $21
  wire sel_musbtn = (regnum[7:4] == 4'h2) && (regnum[1:0] == 2'b10); // $22
  wire sel_kj     = (regnum[7:4] == 4'h2) && (regnum[1:0] == 2'b11); // $23

  wire sel_rstreg = ((regnum[7:4] == 4'h3)) ; // $30

  wire sel_waitreg = (regnum[7:4] == 4'h4) && (regnum[1:0] == 2'b00); // $40
  wire sel_gluadr  = (regnum[7:4] == 4'h4) && (regnum[1:0] == 2'b01); // $41
  wire sel_comadr  = (regnum[7:4] == 4'h4) && (regnum[1:0] == 2'b10); // $42

  wire sel_cfg0 = (regnum[7:4] == 4'h5); // $50

  wire kbd_start = sel_kbdstb && scs_n_01;  // !!! this requires change zx.c for good: kbd_start should be issued BEFORE kbd reg transfer, NOT after
  wire kbd_bit_stb = !scs_n && sel_kbdreg && sck_01;

  reg [5:0] kbd_out_cnt;  // [2:0] - 111 is latching strobe, [5:3] - column selector

  always @(posedge fclk)
    if (kbd_start)
      kbd_out_cnt <= 6'b0;
    else if (kbd_bit_stb)
      kbd_out_cnt <= kbd_out_cnt + 6'b1;

  wire int_wtp = scs_n_01 && sel_cfg0 && shift_in[5];
      
  // registers data-in
  reg [7:0] shift_in;   // register for shifting in
  reg [7:0] wait_reg;   // wait data out register
  reg [7:0] cfg0_reg;   // config register

  always @(posedge fclk)
  begin
    if (!scs_n && sck_01)
      shift_in[7:0] <= {sdo, shift_in[7:1]};

    if (scs_n_01 && sel_waitreg)
      wait_reg <= shift_in;

    if (scs_n_01 && sel_cfg0)
      cfg0_reg <= shift_in;

    if (scs_n_01 && sel_rstreg)
      genrst <= shift_in[0];
  end
endmodule
