// PentEvo project (c) NedoPC 2008-2012
//
// most of pentevo ports are here

`include "../include/tune.v"

module zports(

	input  wire        zclk,   // z80 clock
	input  wire        fclk,  // global FPGA clock
	input  wire        rst_n, // system reset

	input  wire        zpos,
	input  wire        zneg,


	input  wire [ 7:0] din,
	output reg  [ 7:0] dout,
	output wire        dataout,
	input  wire [15:0] a,

	input  wire        iorq_n,
	input  wire        mreq_n,
	input  wire        m1_n,
	input  wire        rd_n,
	input  wire        wr_n,

	output reg         porthit, // when internal port hit occurs, this is 1, else 0; used for iorq1_n iorq2_n on zxbus
	output reg         external_port, // asserts for AY and VG93 accesses

	output wire [15:0] ideout,
	input  wire [15:0] idein,
	output wire        idedataout, // IDE must IN data from IDE device when idedataout=0, else it OUTs
	output wire [ 2:0] ide_a,
	output wire        ide_cs0_n,
	output wire        ide_cs1_n,
	output wire        ide_rd_n,
	output wire        ide_wr_n,


	input  wire [ 4:0] keys_in, // keys (port FE)
	input  wire [ 7:0] mus_in,  // mouse (xxDF)
	input  wire [ 4:0] kj_in,

	output reg  [ 3:0] border,


	input  wire        dos,


	output wire        ay_bdir,
	output wire        ay_bc1,

	output wire [ 7:0] p7ffd,
	output wire [ 7:0] peff7,

	input  wire        tape_read,

	output wire        vg_cs_n,
	input  wire        vg_intrq,
	input  wire        vg_drq, // from vg93 module - drq + irq read
	output wire        vg_wrFF,        // write strobe of #FF port

	output wire        sd_cs_n_val,
	output wire        sd_cs_n_stb,
	output wire        sd_start,
	output wire [ 7:0] sd_datain,
	input  wire [ 7:0] sd_dataout,

	// WAIT-ports related
	//
	output reg  [ 7:0] gluclock_addr,
	//
	output reg  [ 2:0] comport_addr,
	//
	output wire        wait_start_gluclock, // begin wait from some ports
	output wire        wait_start_comport,  //
	//
	output reg         wait_rnw,   // whether it was read(=1) or write(=0)
	output reg  [ 7:0] wait_write,
	input  wire [ 7:0] wait_read,


	output wire        atmF7_wr_fclk, // used in atm_pager.v


	output reg  [ 2:0] atm_scr_mode, // RG0..RG2 in docs
	output reg         atm_turbo,    // turbo mode ON
	output reg         atm_pen,      // pager_off in atm_pager.v, NOT inverted!!!
	output reg         atm_cpm_n,    // permanent dos on
	output reg         atm_pen2,     // PEN2 - fucking palette mode, NOT inverted!!!

	output wire        romrw_en, // from port BF


	output wire        pent1m_ram0_0, // d3.eff7
	output wire        pent1m_1m_on,  // d2.eff7
	output wire [ 5:0] pent1m_page,   // full 1 meg page number
	output wire        pent1m_ROM,     // d4.7ffd


	output wire        atm_palwr,   // palette write strobe
	output wire [ 5:0] atm_paldata, // palette write data

	output wire        covox_wr,
	output wire        beeper_wr,

	output wire        clr_nmi,

	output wire        fnt_wr,		// write to font_ram enabled

	// inputs from atm_pagers, to read back its config
	input  wire [63:0] pages,
	input  wire [ 7:0] ramnroms,
	input  wire [ 7:0] dos7ffds,

	input  wire [ 5:0] palcolor,
	input  wire [ 7:0] fontrom_readback,


	// NMI generation
	output reg         set_nmi,

	// break enable & address
	output reg         brk_ena,
	output reg  [15:0] brk_addr 
);


`define IS_NIDE_REGS(x) ( (x[2:0]==3'b000) && (x[3]!=x[4]) ) 
`define IS_NIDE_HIGH(x) ( x[7:0]==8'h11 )
`define IS_PORT_NIDE(x) ( `IS_NIDE_REGS(x) || `IS_NIDE_HIGH(x) )
`define NIDE_REGS 8'h10,8'h30,8'h50,8'h70,8'h90,8'hB0,8'hD0,8'hF0, \
                  8'h08,8'h28,8'h48,8'h68,8'h88,8'hA8,8'hC8,8'hE8

	localparam PORTFE = 8'hFE;
	localparam PORTF6 = 8'hF6;
	localparam PORTF7 = 8'hF7;

	localparam NIDE10 = 8'h10;
	localparam NIDE11 = 8'h11;
	localparam NIDE30 = 8'h30;
	localparam NIDE50 = 8'h50;
	localparam NIDE70 = 8'h70;
	localparam NIDE90 = 8'h90;
	localparam NIDEB0 = 8'hB0;
	localparam NIDED0 = 8'hD0;
	localparam NIDEF0 = 8'hF0;
	localparam NIDEC8 = 8'hC8;

	localparam PORTFD = 8'hFD;

	localparam VGCOM  = 8'h1F;
	localparam VGTRK  = 8'h3F;
	localparam VGSEC  = 8'h5F;
	localparam VGDAT  = 8'h7F;
	localparam VGSYS  = 8'hFF;

	localparam SAVPORT1 = 8'h2F;
	localparam SAVPORT2 = 8'h4F;
	localparam SAVPORT3 = 8'h6F;
	localparam SAVPORT4 = 8'h8F;

	localparam KJOY   = 8'h1F;
	localparam KMOUSE = 8'hDF;

	localparam SDCFG  = 8'h77;
	localparam SDDAT  = 8'h57;

	localparam ATMF7  = 8'hF7;
	localparam ATM77  = 8'h77;

	localparam ZXEVBE = 8'hBE; // xxBE config-read and nmi-end port
	localparam ZXEVBF = 8'hBF; // xxBF config port
	localparam ZXEVBRK = 8'hBD; // xxBD breakpoint address port	

	localparam COMPORT = 8'hEF; // F8EF..FFEF - rs232 ports


	localparam COVOX   = 8'hFB;




	reg port_wr;
	reg port_rd;

	reg iowr_reg;
	reg iord_reg;


	reg port_wr_fclk,
	    port_rd_fclk,
	    mem_wr_fclk;

	reg [1:0] iowr_reg_fclk,
	          iord_reg_fclk;

	reg [1:0] memwr_reg_fclk;


	wire [7:0] loa;

	wire portfe_wr;



	wire ideout_hi_wr;
	wire idein_lo_rd;
	reg [7:0] idehiin; // IDE high part read register: low part is read directly to Z80 bus,
	                   // while high part is remembered here
	reg ide_ports; // ide ports selected

	reg ide_rd_trig; // nemo-divide read trigger
	reg ide_rd_latch; // to save state of trigger during read cycle

	reg ide_wrlo_trig,  ide_wrhi_trig;  // nemo-divide write triggers
	reg ide_wrlo_latch, ide_wrhi_latch; // save state during write cycles



	reg  [15:0] idewrreg; // write register, either low or high part is pre-written here,
	                      // while other part is out directly from Z80 bus

	wire [ 7:0] iderdeven; // to control read data from "even" ide ports (all except #11)
	wire [ 7:0] iderdodd;  // read data from "odd" port (#11)



	reg pre_bc1,pre_bdir;

	wire gluclock_on;



	reg  shadow_en_reg; //bit0.xxBF
	reg   romrw_en_reg; //bit1.xxBF
	reg  fntw_en_reg; 	//bit2.xxBF

	wire shadow;



	reg [7:0] portbemux;



	reg [7:0] savport [3:0];





	assign shadow = dos || shadow_en_reg;






	assign loa=a[7:0];

	always @*
	begin
		if( (loa==PORTFE) || (loa==PORTF6) ||
		    (loa==PORTFD) ||

		    `IS_PORT_NIDE(loa) ||
//		    (loa==NIDE10) || (loa==NIDE11) || (loa==NIDE30) || (loa==NIDE50) || (loa==NIDE70) ||
//		    (loa==NIDE90) || (loa==NIDEB0) || (loa==NIDED0) || (loa==NIDEF0) || (loa==NIDEC8) ||

		    (loa==KMOUSE) ||

		    ( (loa==VGCOM)&&shadow ) || ( (loa==VGTRK)&&shadow ) || ( (loa==VGSEC)&&shadow ) || ( (loa==VGDAT)&&shadow ) ||
		    ( (loa==VGSYS)&&shadow ) || ( (loa==KJOY)&&(!shadow) ) ||

    		    ( (loa==SAVPORT1)&&shadow ) || ( (loa==SAVPORT2)&&shadow ) ||
		    ( (loa==SAVPORT3)&&shadow ) || ( (loa==SAVPORT4)&&shadow ) ||


		    ( (loa==PORTF7)&&(!shadow) ) || ( (loa==SDCFG)&&(!shadow) ) || ( (loa==SDDAT) ) ||

		    ( (loa==ATMF7)&&shadow ) || ( (loa==ATM77)&&shadow ) ||

		    ( loa==ZXEVBF ) || ( loa==ZXEVBE) || ( loa==ZXEVBRK) || ( loa==COMPORT )
		  )



			porthit = 1'b1;
		else
			porthit = 1'b0;
	end

	always @*
	begin
		if( ((loa==PORTFD) && a[15]) || // 0xBFFD/0xFFFD ports
		    (( (loa==VGCOM)&&shadow ) || ( (loa==VGTRK)&&shadow ) || ( (loa==VGSEC)&&shadow ) || ( (loa==VGDAT)&&shadow )) ) // vg93 ports
			external_port = 1'b1;
		else
			external_port = 1'b0;
	end

	assign dataout = porthit & (~iorq_n) & (~rd_n) & (~external_port);



	// this is zclk-synchronous strobes
	always @(posedge zclk)
	begin
		iowr_reg <= ~(iorq_n | wr_n);
		iord_reg <= ~(iorq_n | rd_n);

		if( (!iowr_reg) && (!iorq_n) && (!wr_n) )
			port_wr <= 1'b1;
		else
			port_wr <= 1'b0;


		if( (!iord_reg) && (!iorq_n) && (!rd_n) )
			port_rd <= 1'b1;
		else
			port_rd <= 1'b0;
	end




	// fclk-synchronous stobes
	//
	always @(posedge fclk) if( zpos )
	begin
		iowr_reg_fclk[0] <= ~(iorq_n | wr_n);
		iord_reg_fclk[0] <= ~(iorq_n | rd_n);
	end

	always @(posedge fclk)
	begin
		iowr_reg_fclk[1] <= iowr_reg_fclk[0];
		iord_reg_fclk[1] <= iord_reg_fclk[0];
	end

	always @(posedge fclk)
	begin
		port_wr_fclk <= iowr_reg_fclk[0] && (!iowr_reg_fclk[1]);
		port_rd_fclk <= iord_reg_fclk[0] && (!iord_reg_fclk[1]);
	end

	always @(posedge fclk)
		memwr_reg_fclk[1:0] <= { memwr_reg_fclk[0], ~(mreq_n | wr_n) };

	always @(posedge fclk)
		mem_wr_fclk <= memwr_reg_fclk[0] && (!memwr_reg_fclk[1]);



	// dout data
	always @*
	begin
		case( loa )
		PORTFE:
			dout = { 1'b1, tape_read, 1'b0, keys_in };
		PORTF6:
			dout = { 1'b1, tape_read, 1'b0, keys_in };


		`NIDE_REGS:
			dout = iderdeven;
		NIDE11:
			dout = iderdodd;


		//PORTFD:

		VGSYS:
			dout = { vg_intrq, vg_drq, 6'b111111 };

		SAVPORT1, SAVPORT2, SAVPORT3, SAVPORT4:
			dout = savport[ loa[6:5] ];


		KJOY:
			dout = {3'b000, kj_in};
		KMOUSE:
			dout = mus_in;

		SDCFG:
			dout = 8'h00; // always SD inserted, SD is in R/W mode
		SDDAT:
			dout = sd_dataout;


		PORTF7: begin
			if( !a[14] && (a[8]^shadow) && gluclock_on ) // $BFF7 - data i/o
				dout = wait_read;
			else // any other $xxF7 port
				dout = 8'hFF;
		end

		COMPORT: begin
			dout = wait_read; // $F8EF..$FFEF
		end

		ZXEVBF: begin
			dout = { 3'b000, brk_ena, set_nmi, fntw_en_reg, romrw_en_reg, shadow_en_reg };
		end

		ZXEVBE: begin
			dout = portbemux;
		end


		default:
			dout = 8'hFF;
		endcase
	end



	assign portfe_wr    = (((loa==PORTFE) || (loa==PORTF6)) && port_wr);
	assign portfd_wr    = ( (loa==PORTFD) && port_wr);

	// F7 ports (like EFF7) are accessible in shadow mode but at addresses like EEF7, DEF7, BEF7 so that
	// there are no conflicts in shadow mode with ATM xFF7 and x7F7 ports
	assign portf7_wr    = ( (loa==PORTF7) && (a[8]==1'b1) && port_wr && (!shadow) ) ||
	                      ( (loa==PORTF7) && (a[8]==1'b0) && port_wr &&   shadow  ) ;

	assign portf7_rd    = ( (loa==PORTF7) && (a[8]==1'b1) && port_rd && (!shadow) ) ||
	                      ( (loa==PORTF7) && (a[8]==1'b0) && port_rd &&   shadow  ) ;

	assign vg_wrFF = ( ( (loa==VGSYS)&&shadow ) && port_wr);

	assign comport_wr   = ( (loa==COMPORT) && port_wr);
	assign comport_rd   = ( (loa==COMPORT) && port_rd);

	
	assign zxevbrk_wr_fclk = ( (loa==ZXEVBRK) && port_wr_fclk);





	// break address write
	always @(posedge fclk)
	if( zxevbrk_wr_fclk)
	begin
		if( !a[8] )
			brk_addr[ 7:0] <= din;
		else // a[8]==1
			brk_addr[15:8] <= din;
	end





	//border port FE
	wire portwe_wr_fclk;

	assign portfe_wr_fclk = (((loa==PORTFE) || (loa==PORTF6)) && port_wr_fclk);

	always @(posedge fclk)
	if( portfe_wr_fclk )
		border <= { ~a[3], din[2:0] };






	// IDE ports

	// IDE physical ports (that go to IDE device)
	always @(loa)
	if( `IS_NIDE_REGS(loa) )
		ide_ports = 1'b1;
	else
		ide_ports = 1'b0;


	assign idein_lo_rd  = port_rd && (loa==NIDE10) && (!ide_rd_trig);

	// control read & write triggers, which allow nemo-divide mod to work.
	//
	// read trigger:
	always @(posedge zclk)
	begin
		if( (loa==NIDE10) && port_rd && !ide_rd_trig )
			ide_rd_trig <= 1'b1;
		else if( ( ide_ports || (loa==NIDE11) ) && ( port_rd || port_wr ) )
			ide_rd_trig <= 1'b0;
	end
	//
	// two triggers for write sequence...
	always @(posedge zclk)
	if( ( ide_ports || (loa==NIDE11) ) && ( port_rd || port_wr ) )
	begin
		if( (loa==NIDE11) && port_wr )
			ide_wrhi_trig <= 1'b1;
		else
			ide_wrhi_trig <= 1'b0;
		//
		if( (loa==NIDE10) && port_wr && !ide_wrhi_trig && !ide_wrlo_trig )
			ide_wrlo_trig <= 1'b1;
		else
			ide_wrlo_trig <= 1'b0;
	end

	// normal read: #10(low), #11(high)
	// divide read: #10(low), #10(high)
	//
	// normal write: #11(high), #10(low)
	// divide write: #10(low),  #10(high)


	always @(posedge zclk)
	begin
		if( port_wr && (loa==NIDE11) )
			idewrreg[15:8] <= din;

		if( port_wr && (loa==NIDE10) && !ide_wrlo_trig )
			idewrreg[ 7:0] <= din;
	end




	always @(posedge zclk)
	if( idein_lo_rd )
			idehiin <= idein[15:8];


	assign ide_a = a[7:5];


	// This is unknown shit... Probably need more testing with old WD
	// drives WITHOUT this commented fix.
	//
	// trying to fix old WD drives...
	//assign ide_cs0_n = iorq_n | (rd_n&wr_n) | (~ide_ports) | (~(loa!=NIDEC8));
	//assign ide_cs1_n = iorq_n | (rd_n&wr_n) | (~ide_ports) | (~(loa==NIDEC8));
	// fix ends...


	assign ide_cs0_n = (~ide_ports) | (~(loa!=NIDEC8));
	assign ide_cs1_n = (~ide_ports) | (~(loa==NIDEC8));


	// generate read cycles for IDE as usual, except for reading #10
	// instead of #11 for high byte (nemo-divide). I use additional latch
	// since 'ide_rd_trig' clears during second Z80 IO read cycle to #10
	always @* if( rd_n ) ide_rd_latch <= ide_rd_trig;
	//
	assign ide_rd_n = iorq_n | rd_n | (~ide_ports) | (ide_rd_latch && (loa==NIDE10));

	always @* if( wr_n ) ide_wrlo_latch <= ide_wrlo_trig; // same for write triggers
	always @* if( wr_n ) ide_wrhi_latch <= ide_wrhi_trig; //
	//
	assign ide_wr_n = iorq_n | wr_n | (~ide_ports) | ( (loa==NIDE10) && !ide_wrlo_latch && !ide_wrhi_latch );
	                                          // do NOT generate IDE write, if neither of ide_wrhi|lo latches
	                                          // set and writing to NIDE10



//	assign idedataout = ide_rd_n;
	assign idedataout = ~ide_wr_n; // shit-fix in try to fix IDE errors
	// warning: this fix kinda blind-picking, good way is to
	// have idedataout lead wr or rd strobes. also good point to disable data ringing
	// on ide data bus while not accessing IDE


	// data read by Z80 from IDE
	//
	assign iderdodd[ 7:0] = idehiin[ 7:0];
	//
	assign iderdeven[ 7:0] = (ide_rd_latch && (loa==NIDE10)) ? idehiin[ 7:0] : idein[ 7:0];

	// data written to IDE from Z80
	//
	assign ideout[15:8] = ide_wrhi_latch ? idewrreg[15:8] : din[ 7:0];
	assign ideout[ 7:0] = ide_wrlo_latch ? idewrreg[ 7:0] : din[ 7:0];







	// AY control
	always @*
	begin
		pre_bc1 = 1'b0;
		pre_bdir = 1'b0;

		if( loa==PORTFD )
		begin
			if( a[15:14]==2'b11 )
			begin
				pre_bc1=1'b1;
				pre_bdir=1'b1;
			end
			else if( a[15:14]==2'b10 )
			begin
				pre_bc1=1'b0;
				pre_bdir=1'b1;
			end
		end
	end

	assign ay_bc1  = pre_bc1  & (~iorq_n) & ((~rd_n)|(~wr_n));
	assign ay_bdir = pre_bdir & (~iorq_n) & (~wr_n);



	// 7FFD port
	reg [7:0] p7ffd_int,peff7_int;
	reg p7ffd_rom_int;
	wire block7ffd;
	wire block1m;

	always @(posedge zclk, negedge rst_n)
	begin
		if( !rst_n )
			p7ffd_int <= 7'h00;
		else if( (a[15]==1'b0) && portfd_wr && (!block7ffd) )
			p7ffd_int <= din; // 2..0 - page, 3 - screen, 4 - rom, 5 - block48k, 6..7 -
	end

	always @(posedge zclk, negedge rst_n)
	if( !rst_n )
			p7ffd_rom_int <= 1'b0;
	else
		if( (a[15]==1'b0) && portfd_wr && (!block7ffd) )
			p7ffd_rom_int <= din[4];


	assign block7ffd=p7ffd_int[5] & block1m;


	// EFF7 port
	always @(posedge zclk, negedge rst_n)
	begin
		if( !rst_n )
			peff7_int <= 8'h00;
		else if( !a[12] && portf7_wr && (!shadow) ) // EEF7 in shadow mode is abandoned!
			peff7_int <= din; // 4 - turbooff, 0 - p16c on, 2 - block1meg
	end
	assign block1m = peff7_int[2];

	assign p7ffd = { (block1m ? 3'b0 : p7ffd_int[7:5]),p7ffd_rom_int,p7ffd_int[3:0]};

	assign peff7 = block1m ? { peff7_int[7], 1'b0, peff7_int[5], peff7_int[4], 3'b000, peff7_int[0] } : peff7_int;


	assign pent1m_ROM       = p7ffd_int[4];
	assign pent1m_page[5:0] = { p7ffd_int[7:5], p7ffd_int[2:0] };
	assign pent1m_1m_on     = ~peff7_int[2];
	assign pent1m_ram0_0    = peff7_int[3];




	// gluclock ports (bit7:eff7 is above)

	assign gluclock_on = peff7_int[7] || shadow; // in shadow mode EEF7 is abandoned: instead, gluclock access
	                                             // is ON forever in shadow mode.

	always @(posedge zclk)
	begin
		if( gluclock_on && portf7_wr ) // gluclocks on
		begin
			if( !a[13] ) // $DFF7 - addr reg
				gluclock_addr <= din;

			// write to waiting register is not here - in separate section managing wait_write
		end
	end


	// comports

	always @(posedge zclk)
	begin
		if( comport_wr || comport_rd )
			comport_addr <= a[10:8 ];
	end



	// write to wait registers
	always @(posedge zclk)
	begin
		// gluclocks
		if( gluclock_on && portf7_wr && !a[14] ) // $BFF7 - data reg
			wait_write <= din;
		// com ports
		else if( comport_wr ) // $F8EF..$FFEF - comports
			wait_write <= din;
	end

	// wait from wait registers
	//
	// ACHTUNG!!!! here portxx_wr are ON Z80 CLOCK! logic must change when moving to fclk strobes
	//
	assign wait_start_gluclock = ( gluclock_on && !a[14] && (portf7_rd || portf7_wr) ); // $BFF7 - gluclock r/w
	//
	assign wait_start_comport = ( comport_rd || comport_wr );
	//
	//
	always @(posedge zclk) // wait rnw - only meanful during wait
	begin
		if( port_wr )
			wait_rnw <= 1'b0;

		if( port_rd )
			wait_rnw <= 1'b1;
	end





	// VG93 control
	assign vg_cs_n =  (~shadow) | iorq_n | (rd_n & wr_n) | ( ~((loa==VGCOM)|(loa==VGTRK)|(loa==VGSEC)|(loa==VGDAT)) );








// SD card (z-control¸r compatible)

	wire sdcfg_wr,sddat_wr,sddat_rd;

	assign sdcfg_wr = ( (loa==SDCFG) && port_wr_fclk && (!shadow) )                  ||
	                  ( (loa==SDDAT) && port_wr_fclk &&   shadow  && (a[15]==1'b1) ) ;

	assign sddat_wr = ( (loa==SDDAT) && port_wr_fclk && (!shadow) )                  ||
	                  ( (loa==SDDAT) && port_wr_fclk &&   shadow  && (a[15]==1'b0) ) ;

	assign sddat_rd = ( (loa==SDDAT) && port_rd_fclk              );

	// SDCFG write - sdcs_n control
	assign sd_cs_n_stb = sdcfg_wr;
	assign sd_cs_n_val = din[1];


	// start signal for SPI module with resyncing to fclk

	assign sd_start = sddat_wr || sddat_rd;

	// data for SPI module
	assign sd_datain = sddat_rd ? 8'hFF : din;







/////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////
	// ATM ports //
	///////////////

	wire atm77_wr_fclk;
	wire zxevbf_wr_fclk;

	assign atmF7_wr_fclk = ( (loa==ATMF7) && (a[8]==1'b1) && shadow && port_wr_fclk ); // xFF7 and x7F7 ports, NOT xEF7!
	assign atm77_wr_fclk = ( (loa==ATM77) && shadow && port_wr_fclk );

	assign zxevbf_wr_fclk = ( (loa==ZXEVBF) && port_wr_fclk );


	// port BF write
	//
	always @(posedge fclk, negedge rst_n)
	if( !rst_n )
	begin
		shadow_en_reg <= 1'b0;
		romrw_en_reg  <= 1'b0;
		fntw_en_reg   <= 1'b0;
		set_nmi       <= 1'b0;
		brk_ena       <= 1'b0;
	end
	else if( zxevbf_wr_fclk )
	begin
		shadow_en_reg <= din[0];
		romrw_en_reg  <= din[1];
		fntw_en_reg   <= din[2];
		set_nmi       <= din[3];
		brk_ena       <= din[4];
	end

	assign romrw_en = romrw_en_reg;



	// port xx77 write
	always @(posedge fclk, negedge rst_n)
	if( !rst_n )
	begin
		atm_scr_mode = 3'b011;
		atm_turbo    = 1'b0;

		atm_pen =   1'b1; // no manager,
		atm_cpm_n = 1'b0; // permanent dosen (shadow ports on)


		atm_pen2     = 1'b0;
	end
	else if( atm77_wr_fclk )
	begin
		atm_scr_mode <= din[2:0];
		atm_turbo    <= din[3];
		atm_pen      <= ~a[8];
		atm_cpm_n    <=  a[9];
		atm_pen2     <= ~a[14];
	end


	// atm palette strobe and data
	wire vg_wrFF_fclk;

	assign vg_wrFF_fclk = ( ( (loa==VGSYS)&&shadow ) && port_wr_fclk);


	assign atm_palwr = vg_wrFF_fclk & atm_pen2;

	assign atm_paldata = { ~din[4], ~din[7], ~din[1], ~din[6], ~din[0], ~din[5] };



	// port BE write
	assign clr_nmi = ( (loa==ZXEVBE) && port_wr_fclk );




	// covox/beeper writes

	assign beeper_wr = (loa==PORTFE) && portfe_wr_fclk;
	assign covox_wr  = (loa==COVOX) && port_wr_fclk;



	// font write enable
	assign fnt_wr = fntw_en_reg && mem_wr_fclk;



	// port BE read

	always @*
	case( a[12:8] )

	5'h0: portbemux = pages[ 7:0 ];
	5'h1: portbemux = pages[15:8 ];
	5'h2: portbemux = pages[23:16];
	5'h3: portbemux = pages[31:24];
	5'h4: portbemux = pages[39:32];
	5'h5: portbemux = pages[47:40];
	5'h6: portbemux = pages[55:48];
	5'h7: portbemux = pages[63:56];

	5'h8: portbemux = ramnroms;
	5'h9: portbemux = dos7ffds;

	5'hA: portbemux = p7ffd_int;
	5'hB: portbemux = peff7_int;

	5'hC: portbemux = { ~atm_pen2, atm_cpm_n, ~atm_pen, dos, atm_turbo, atm_scr_mode };

	5'hD: portbemux = { ~palcolor[4], ~palcolor[2], ~palcolor[0], ~palcolor[5], 2'b11, ~palcolor[3], ~palcolor[1] };
//	assign atm_paldata = { ~din[4], ~din[7], ~din[1], ~din[6], ~din[0], ~din[5] };
//  {GgRrBb} -> {grbG11RB}
// was: 76543210 -> 471605
// now:             543210 -> 4205xx31

	5'hE: portbemux = fontrom_readback;

	5'h10: portbemux = brk_addr[7:0];
	5'h11: portbemux = brk_addr[15:8];

	default: portbemux = 8'bXXXXXXXX;

	endcase





	// savelij ports write
	//
	always @(posedge fclk)
	if( port_wr_fclk && shadow )
	begin
		if( (loa==SAVPORT1) ||
		    (loa==SAVPORT2) ||
		    (loa==SAVPORT3) ||
		    (loa==SAVPORT4) )
			savport[ loa[6:5] ] <= din;
	end



endmodule

