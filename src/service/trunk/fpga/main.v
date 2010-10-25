module main(

    // clocks
    input fclk,
    output clkz_out,
    input clkz_in,

    // z80
    input iorq_n,
    input mreq_n,
    input rd_n,
    input wr_n,
    input m1_n,
    input rfsh_n,
    input int_n,
    input nmi_n,
    input wait_n,
    output res,

    inout [7:0] d,
    output [15:0] a,

    // zxbus and related
    output csrom,
    output romoe_n,
    output romwe_n,

    output rompg0_n,
    output dos_n, // aka rompg1
    output rompg2,
    output rompg3,
    output rompg4,

    input iorqge1,
    input iorqge2,
    output iorq1_n,
    output iorq2_n,

    // DRAM
    inout [15:0] rd,
    output [9:0] ra,
    output rwe_n,
    output rucas_n,
    output rlcas_n,
    output rras0_n,
    output rras1_n,

    // video
    output reg [1:0] vred,
    output reg [1:0] vgrn,
    output reg [1:0] vblu,

    output vhsync,
    output vvsync,
    output vcsync,

    // AY control and audio/tape
    input ay_clk,
    output ay_bdir,
    output ay_bc1,

//    output beep,
    output reg beep,

    // IDE
    input [2:0] ide_a,
    input [15:0] ide_d,

    output ide_dir,

    input ide_rdy,

    output ide_cs0_n,
    output ide_cs1_n,
    output ide_rs_n,
    output ide_rd_n,
    output ide_wr_n,

    // VG93 and diskdrive
    output reg vg_clk,

    output vg_cs_n,
    output vg_res_n,

    input vg_hrdy,
    input vg_rclk,
    input vg_rawr,
    input [1:0] vg_a, // disk drive selection
    input vg_wrd,
    input vg_side,

    input step,
    input vg_sl,
    input vg_sr,
    input vg_tr43,
    input rdat_b_n,
    input vg_wf_de,
    input vg_drq,
    input vg_irq,
    input vg_wd,

    // serial links (atmega-fpga, sdcard)
    output sdcs_n,
    output sddo,
    output sdclk,
    input sddi,

    input spics_n,
    input spick,
    input spido,
    output spidi,
    output reg spiint_n
);

//--Dummy----------------------------------------------------------------------

    assign iorq1_n = 1'b1;
    assign iorq2_n = 1'b1;

    assign res= 1'b1;

    assign ay_bdir = 1'b0;
    assign ay_bc1  = 1'b0;

    assign vg_cs_n  = 1'b1;
    assign vg_res_n = 1'b0;

    assign ide_dir=1'b1;
    assign ide_rs_n = 1'b0;
    assign ide_cs0_n = 1'b1;
    assign ide_cs1_n = 1'b1;
    assign ide_rd_n = 1'b1;
    assign ide_wr_n = 1'b1;

    assign a[15:14] = 2'b00;

//--INT---------------------------------------------------------------------------

 reg enable_covox_int;  initial enable_covox_int = 1'b0;
 reg enable_frame_int;  initial enable_frame_int = 1'b0;

 always @(posedge fclk)
  begin
   if ( enable_covox_int )
    spiint_n <= ( main_osc[9:2]==8'h00 ) ? 1'b0 : 1'b1;
   else if ( enable_frame_int )
    spiint_n <= ~vblank;
   else
    spiint_n <= 1'b1;
  end

//--clocks--Z80_clk--VG_clk--COVOX---------------------------------------------

 reg [9:0] main_osc;
 reg [7:0] covox;       initial covox = 8'h7f;
 reg [7:0] covox_work;
 reg [2:0] vgclk_div7;

 always @(posedge fclk)
  begin
   //
   main_osc <= main_osc + 10'h01;
   //
   if ( main_osc[7:0] < covox_work )
    beep <= 1'b1;
   else
    beep <= 1'b0;
   //
   if ( main_osc[7:0]==8'h00 )
    covox_work <= covox;
   //
   if ( main_osc[1:0]==2'b00 )
   begin
    if ( vgclk_div7[2:1] == 2'b11 )
     begin
      vgclk_div7 <= 3'd0;
      vg_clk <= ~vg_clk;
     end
    else
     vgclk_div7 <= vgclk_div7 + 3'd1;
   end
   //
  end

 assign clkz_out = main_osc[2]; // 3.5 MHz

//--Video----------------------------------------------------------------------
// character image - 6x8
// character size  - 6x10
// col x row       - 53x25 (318x250)

 localparam HBLNK_BEG  = 9'd416;
 localparam CSYNC_CUT  = 9'd415;
 localparam HSYNC_BEG  = 9'd0;
 localparam HSYNC_END  = 9'd33;
 localparam HSYNC_END2 = 9'd53;
 localparam HBLNK_END  = 9'd98;
 localparam HMAX       = 9'd447;

 localparam VBLNK_BEG  = 9'd294;
 localparam VSYNC_BEG  = 9'd0;
 localparam VSYNC_END  = 9'd2;
 localparam VBLNK_END  = 9'd44;
 localparam VMAX       = 9'd319;

 localparam HMARK_A    = 9'd77;
 localparam HMARK_B    = 9'd437;
 localparam VMARK_A    = 9'd25;
 localparam VMARK_B    = 9'd313;

 reg [8:0] hcount;      initial hcount = 9'd0;
 reg [2:0] pixptr;
 reg [8:0] vcount;      initial vcount = 9'd0;
 reg [5:0] hcharcount;
 wire [2:0] vcharline;
 reg [10:0] voffset;
 reg [3:0] vcharlinecount;
 reg hsync;             initial hsync = 1'b1;
 reg hblank;            initial hblank = 1'b1;
 reg vsync;             initial vsync = 1'b1;
 reg vblank;            initial vblank = 1'b1;
 reg csync;             initial csync = 1'b1;
 wire [10:0] video_addr;
 wire [7:0] charcode0, charcode1, charcode2;
 wire [7:0] charcode;
 wire [7:0] attrcode0, attrcode1, attrcode2;
 wire [7:0] attrcode;
 wire [5:0] charpix;
 wire pixel;
 wire [5:0] fcolor, bcolor, color, image_color;
 wire fontenable;
 reg [8:0] hmouse, vmouse;
 wire mouse_here, mouse_i, mouse_image, mouse_m, mouse_mask;
 reg vgaff, nextline;
 reg hmark;             initial hmark = 1'b0;
 reg vmark;             initial vmark = 1'b0;

 always @(posedge fclk)
  begin
   //
   if ( {(main_osc[1]&scr_tv_mode),main_osc[0]}==2'h0 )
    begin

     if ( hblank )
      begin
       hcharcount <= 6'h00;
       pixptr <= 3'd0;
      end
     else
      begin
       if ( pixptr==3'd5 )
        begin
         pixptr <= 3'd0;
         hcharcount <= hcharcount + 6'h01;
        end
       else
        pixptr <= pixptr + 3'd1;
      end

     if ( hcount==HMAX )
      hcount <= 9'd0;
     else
      hcount <= hcount + 9'd1;

     if ( hcount==HBLNK_BEG )
      hblank <= 1'b1;
     else if ( hcount==HBLNK_END )
      hblank <= 1'b0;

     if ( hcount==HSYNC_BEG )
      begin
       hsync <= 1'b1;
       vgaff <= scr_tv_mode | ~vgaff;
       if ( vgaff )
        begin
         csync <= 1'b1;
         nextline <= 1'b1;
        end
      end

     if ( (~scr_tv_mode) && (hcount==HSYNC_END2) )
      begin
       hsync <= 1'b0;
       if ( !vsync )
        csync <= 1'b0;
      end

     if ( scr_tv_mode && (hcount==HSYNC_END) )
      begin
       hsync <= 1'b0;
       if ( !vsync )
        csync <= 1'b0;
      end

     if ( (vgaff) && (hcount==HBLNK_BEG) ) // localparam CSYNC_CUT2 = 9'd382;
      csync <= 1'b0;

     if ( scr_tv_mode && (hcount==CSYNC_CUT) )
      csync <= 1'b0;

     if ( hcount==HMARK_A )
      hmark <= 1'b1;
     else if ( hcount==HMARK_B )
      hmark <= 1'b0;

     vgrn[1] <= color[5];
     vgrn[0] <= color[4];
     vred[1] <= color[3];
     vred[0] <= color[2];
     vblu[1] <= color[1];
     vblu[0] <= color[0];

     hmouse <= hcount - scr_mouse_x;

    end
   //
   if ( nextline )
    begin

     nextline <= 1'b0;

     if ( vblank )
      begin
       voffset <= 11'd0;
       vcharlinecount <= 4'd15;
      end
     else
      begin
       if ( vcharlinecount==4'd8 )
        begin
         voffset <= voffset + 11'd53;
         vcharlinecount <= 4'd15;
        end
       else
        vcharlinecount <= vcharlinecount + 4'd1;
      end

     if ( vcount==VMAX )
      begin
       vcount <= 9'd0;
//       cursorflash <= cursorflash + 4'd1;
      end
     else
      vcount <= vcount + 9'd1;

     if ( vcount==VBLNK_BEG )
      vblank <= 1'b1;
     else if ( vcount==VBLNK_END )
      vblank <= 1'b0;

     if ( vcount==VSYNC_BEG )
      vsync <= 1'b1;
     else if ( vcount==VSYNC_END )
      vsync <= 1'b0;

     if ( vcount==VMARK_A )
      vmark <= 1'b1;
     else if ( vcount==VMARK_B )
      vmark <= 1'b0;

     vmouse <= vcount - scr_mouse_y;

    end
   //
  end

 lpm_rom_7x2 mouse_cursor ( .address({ vmouse[3:0], hmouse[2:0] }), .q({ mouse_i, mouse_m }) );
 assign mouse_here = (hmouse[8:3] == 6'd0) && (vmouse[8:4] == 5'd0);
 assign mouse_mask = mouse_here & mouse_m;
 assign mouse_image = mouse_here & mouse_i;

 assign video_addr = voffset + { 4'h0, hcharcount[5:0] };
 lpm_ram_dp_9x8 scrmem0  ( .data(indata), .wraddress(scr_addr[8:0]), .wren((scr_wren_c)&&(scr_addr[10:9]==2'h0)),
                           .rdaddress(video_addr[8:0]), .q(charcode0) );
 lpm_ram_dp_9x8 scrmem1  ( .data(indata), .wraddress(scr_addr[8:0]), .wren((scr_wren_c)&&(scr_addr[10:9]==2'h1)),
                           .rdaddress(video_addr[8:0]), .q(charcode1) );
 lpm_ram_dp_9x8 scrmem2  ( .data(indata), .wraddress(scr_addr[8:0]), .wren((scr_wren_c)&&(scr_addr[10:9]==2'h2)),
                           .rdaddress(video_addr[8:0]), .q(charcode2) );
 assign charcode = (video_addr[10:9]==2'h0) ? charcode0 :
                   (video_addr[10:9]==2'h1) ? charcode1 : charcode2 ;
 lpm_ram_dp_9x8 attrmem0 ( .data(scr_attr), .wraddress(scr_addr[8:0]), .wren((scr_wren_a)&&(scr_addr[10:9]==2'h0)),
                           .rdaddress(video_addr[8:0]), .q(attrcode0) );
 lpm_ram_dp_9x8 attrmem1 ( .data(scr_attr), .wraddress(scr_addr[8:0]), .wren((scr_wren_a)&&(scr_addr[10:9]==2'h1)),
                           .rdaddress(video_addr[8:0]), .q(attrcode1) );
 lpm_ram_dp_9x8 attrmem2 ( .data(scr_attr), .wraddress(scr_addr[8:0]), .wren((scr_wren_a)&&(scr_addr[10:9]==2'h2)),
                           .rdaddress(video_addr[8:0]), .q(attrcode2) );
 assign attrcode = (video_addr[10:9]==2'h0) ? attrcode0 :
                   (video_addr[10:9]==2'h1) ? attrcode1 : attrcode2 ;
 assign vcharline = (vcharlinecount[3]) ? ~vcharlinecount[2:0] : vcharlinecount[2:0];
 lpm_rom_11x6 chargen ( .address({ charcode, vcharline }), .q(charpix) );

 assign fcolor = { attrcode[2], (attrcode[2]&attrcode[3]),
                   attrcode[1], (attrcode[1]&attrcode[3]),
                   attrcode[0], (attrcode[0]&attrcode[3]) };
 assign bcolor = { (attrcode[6]&attrcode[7]), (attrcode[6]&(~attrcode[7])),
                   (attrcode[5]&attrcode[7]), (attrcode[5]&(~attrcode[7])),
                   (attrcode[4]&attrcode[7]), (attrcode[4]&(~attrcode[7])) };
 assign fontenable = (charcode[7:4]==4'hb)||
                     (charcode[7:4]==4'hc)||
                     (charcode[7:4]==4'hd)||
                     (~vcharlinecount[3]);
 assign pixel = ( charcode==8'hb0 ) ? ( (vcount[0]^hcount[1])&~hcount[0] ) :
                ( charcode==8'hb1 ) ? (  vcount[0]^hcount[0]             ) :
                ( charcode==8'hb2 ) ? ( (vcount[0]^hcount[1])| hcount[0] ) :
                ( fontenable ) ? charpix[3'd5-pixptr] : 1'b0;

 assign mark = scr_mark&vmark&hmark&(vcount[0]^hcount[0]);

 assign image_color = (mouse_mask) ? 6'h00 : ( pixel ? fcolor : bcolor ) ;
 assign color = (hblank | vblank) ? ((mark) ? 6'h3f : 6'h00) :
                (mouse_image) ? ~image_color : image_color;

 assign vhsync = hsync;
 assign vvsync = vsync;
 assign vcsync = ~csync;

//--AVRSPI--FlashROM-----------------------------------------------------------

 localparam TEMP_REG      = 8'ha0;

 localparam SD_CS0        = 8'ha1;
 localparam SD_CS1        = 8'ha2;
 localparam FLASH_LOADDR  = 8'ha3;
 localparam FLASH_MIDADDR = 8'ha4;
 localparam FLASH_HIADDR  = 8'ha5;
 localparam FLASH_DATA    = 8'ha6;
 localparam FLASH_CTRL    = 8'ha7;
 localparam SCR_LOADDR    = 8'ha8;
 localparam SCR_HIADDR    = 8'ha9; // ...и управление курсором (скрыть/показать)
 localparam SCR_SET_ATTR  = 8'haa; // запись в ATTR
 localparam SCR_FILL      = 8'hab; // прединкремент адреса и запись в ATTR и в память
                                   // (если только дергать spics_n, то в память будет писаться предыдущее значение)
 localparam SCR_CHAR      = 8'hac; // прединкремент адреса и запись в память символов и ATTR в память атрибутов
                                   // (если только дергать spics_n, то в память будет писаться предыдущие значения)
 localparam SCR_MOUSE_X   = 8'had;
 localparam SCR_MOUSE_Y   = 8'hae;
 localparam SCR_MODE      = 8'haf; // .0 - TV-mode (default==1); .1 - 0=сетка на "бордюре"

 localparam MTST_CONTROL  = 8'h50; // .0 - тест памяти (0 - сброс, 1 - работа)
 localparam MTST_PASS_CNT0= 8'h51;
 localparam MTST_PASS_CNT1= TEMP_REG;
 localparam MTST_FAIL_CNT0= 8'h52;
 localparam MTST_FAIL_CNT1= TEMP_REG;

 localparam COVOX         = 8'h53;

 localparam INT_CONTROL   = 8'h54; // .0 - разрешение прерываний от covox-а (27343.75 Hz)
                                   // .1 - разрешение кадровых прерываний (~50 Hz)

 reg [7:0] number;          initial number = 8'hff;
 reg [7:0] indata;          initial indata = 8'hff;
 reg [7:0] outdata;
 reg [2:0] bitptr;
 reg prev_spics_n;
 reg [18:0] flash_addr;
 reg flash_cs;              initial flash_cs = 1'b0;
 reg flash_oe;              initial flash_oe = 1'b0;
 reg flash_we;              initial flash_we = 1'b0;
 reg [7:0] flash_data_out;
 reg [10:0] scr_addr;       initial scr_addr = 11'h000;
 reg [7:0] scr_attr;        initial scr_attr = 8'h0f;
 reg scr_wren_c;            initial scr_wren_c = 1'b0;
 reg scr_wren_a;            initial scr_wren_a = 1'b0;
 reg [8:0] scr_mouse_x;     initial scr_mouse_x = 9'd0;
 reg [8:0] scr_mouse_y;     initial scr_mouse_y = 9'd0;
 reg scr_tv_mode;           initial scr_tv_mode = 1'b1;
 reg scr_mark;              initial scr_mark = 1'b0;
 wire spicsn_rising;
 wire spicsn_falling;
 wire sd_selected;
 reg cs_trg;
 reg [7:0] temp_reg;

 always @(posedge spick)
  begin
   if ( spics_n )
    number <= { number[6:0], spido };
   else
    indata <= { indata[6:0], spido };
  end

 always @(negedge spick or posedge spics_n)
  begin
   if ( spics_n )
    bitptr <= 3'b111;
   else
    bitptr <= bitptr - 3'b001;
  end

 always @(posedge fclk)
  begin
   //
   if ( spicsn_rising )
    begin
     //
     cs_trg <= 1'b1;
     //
     case ( number )
      FLASH_LOADDR:  flash_addr[7:0] <= indata;
      FLASH_MIDADDR: flash_addr[15:8] <= indata;
      FLASH_HIADDR:  flash_addr[18:16] <= indata[2:0];
      FLASH_DATA:    flash_data_out <= indata;
      FLASH_CTRL:    begin
                      flash_cs <= indata[0];
                      flash_oe <= indata[1];
                      flash_we <= indata[2];
                     end
      SCR_LOADDR:    scr_addr[7:0] <= indata;
      SCR_HIADDR:    scr_addr[10:8] <= indata[2:0];
      SCR_SET_ATTR:  scr_attr <= indata;
      SCR_FILL:      begin
                      scr_attr <= indata;
                      scr_wren_a <= 1'b1;
                     end
      SCR_CHAR:      begin
                      scr_wren_c <= 1'b1;
                      scr_wren_a <= 1'b1;
                     end
      TEMP_REG:      temp_reg <= indata;
      SCR_MOUSE_X:   scr_mouse_x <= { temp_reg[0], indata };
      SCR_MOUSE_Y:   scr_mouse_y <= { temp_reg[0], indata };
      SCR_MODE:      begin
                      scr_tv_mode <= indata[0];
                      scr_mark <= ~indata[1];
                     end
      MTST_CONTROL:  mtst_run <= indata[0];
      COVOX:         covox <= indata;
      INT_CONTROL:   begin
                      enable_covox_int <= indata[0];
                      enable_frame_int <= indata[1];
                     end
     endcase
     //
    end
   else
   begin
    //
    scr_wren_c <= 1'b0;
    scr_wren_a <= 1'b0;
    //
    if ( spicsn_falling )
     begin
      //
      cs_trg <= 1'b0;
      //
      case ( number )
       SCR_FILL:      begin
                       outdata <= 8'hff;
                       scr_addr <= scr_addr + 11'd1;
                      end
       SCR_CHAR:      begin
                       outdata <= 8'hff;
                       scr_addr <= scr_addr + 11'd1;
                      end
       FLASH_DATA:    outdata <= d;
       COVOX:         outdata <= ~covox; // for SPI testing
       MTST_PASS_CNT0:begin
                       outdata <= mtst_pass_counter[7:0];
                       temp_reg <= mtst_pass_counter[15:8];
                      end
       MTST_FAIL_CNT0:begin
                       outdata <= mtst_fail_counter[7:0];
                       temp_reg <= mtst_fail_counter[15:8];
                      end
       TEMP_REG:      outdata <= temp_reg; // read after MTST_PASS_CNT0, MTST_FAIL_CNT0
       default:       outdata <= 8'hff;
      endcase
      //
     end
    //
   end
   //
   prev_spics_n <= spics_n;
   //
  end

 assign spicsn_rising  = ( { cs_trg, prev_spics_n, spics_n } == 3'b011 );
 assign spicsn_falling = ( { cs_trg, prev_spics_n, spics_n } == 3'b100 );

 assign sd_selected = ( ( (number==SD_CS0) || (number==SD_CS1) ) && (~spics_n) );
 assign spidi = sd_selected ? sddi : outdata[bitptr];
 assign sddo  = sd_selected ? spido : 1'b1;
 assign sdclk = sd_selected ? spick : 1'b0;
 assign sdcs_n = !( (number==SD_CS0) && (~spics_n) );

 assign a[13:0]  =  flash_addr[13:0];
 assign rompg0_n = ~flash_addr[14];
 assign { rompg4, rompg3, rompg2, dos_n } = flash_addr[18:15];
 assign csrom   =  flash_cs;
 assign romoe_n = ~flash_oe;
 assign romwe_n = ~flash_we;
 assign d = flash_oe ? 8'bZZZZZZZZ : flash_data_out;

//-----------------------------------------------------------------------------

 reg mtst_run;      initial mtst_run = 1'b0;
 wire [15:0] mtst_pass_counter;
 wire [15:0] mtst_fail_counter;

 mem_tester mtst( .clk(fclk), .rst_n(mtst_run),
                  .pass_counter(mtst_pass_counter),
                  .fail_counter(mtst_fail_counter),
                  .DRAM_DQ(rd), .DRAM_MA(ra), .DRAM_RAS0_N(rras0_n), .DRAM_RAS1_N(rras1_n),
                  .DRAM_LCAS_N(rlcas_n), .DRAM_UCAS_N(rucas_n), .DRAM_WE_N(rwe_n) );

//-----------------------------------------------------------------------------

endmodule
