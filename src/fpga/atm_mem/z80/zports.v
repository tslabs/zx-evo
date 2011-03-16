// PentEvo project (c) NedoPC 2008-2010
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

	output reg  [15:0] ideout,
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

	output reg  [ 2:0] border,
	output reg         beep,

	input  wire        dos,


	output wire        ay_bdir,
	output wire        ay_bc1,

	output wire [ 7:0] p7ffd,
	output wire [ 7:0] peff7,

	input  wire [ 1:0] rstrom,

	output wire        vg_cs_n,
	input  wire        vg_intrq,
	input  wire        vg_drq, // from vg93 module - drq + irq read
	output wire        vg_wrFF,        // write strobe of #FF port

	output reg         sdcs_n,
	output wire        sd_start,
	output wire [ 7:0] sd_datain,
	input  wire [ 7:0] sd_dataout,


	output reg  [ 7:0] gluclock_addr,

	output wire        wait_start_gluclock, // begin wait from some ports
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
	output wire        pent1m_ROM     // d4.7ffd

);


	reg rstsync1,rstsync2;


	localparam PORTFE = 8'hFE;
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

	localparam KJOY   = 8'h1F;
	localparam KMOUSE = 8'hDF;

	localparam SDCFG  = 8'h77;
	localparam SDDAT  = 8'h57;

	localparam ATMF7  = 8'hF7;
	localparam ATM77  = 8'h77;

	localparam ZXEVBF = 8'hBF; // xxBF config port

	reg external_port;

	reg port_wr;
	reg port_rd;

	reg iowr_reg;
	reg iord_reg;


	reg port_wr_fclk,
	    port_rd_fclk;

	reg [1:0] iowr_reg_fclk,
	          iord_reg_fclk;


	wire [7:0] loa;

	wire portfe_wr;

	wire ideout_hi_wr;
	wire idein_lo_rd;
	reg [7:0] idehiin;
	reg ide_ports; // ide ports selected

	reg pre_bc1,pre_bdir;

	wire gluclock_on;



	reg  shadow_en_reg; //bit0.xxBF
	reg   romrw_en_reg; //bit1.xxBF

	wire shadow;





	assign shadow = dos || shadow_en_reg;






	assign loa=a[7:0];

	always @*
	begin
		if( (loa==PORTFE) ||
		    (loa==PORTFD) ||

		    (loa==NIDE10) || (loa==NIDE11) || (loa==NIDE30) || (loa==NIDE50) || (loa==NIDE70) ||
		    (loa==NIDE90) || (loa==NIDEB0) || (loa==NIDED0) || (loa==NIDEF0) || (loa==NIDEC8) ||

		    (loa==KMOUSE) ||

		    ( (loa==VGCOM)&&shadow ) || ( (loa==VGTRK)&&shadow ) || ( (loa==VGSEC)&&shadow ) || ( (loa==VGDAT)&&shadow ) ||
		    ( (loa==VGSYS)&&shadow ) || ( (loa==KJOY)&&(!shadow) ) ||

		    ( (loa==PORTF7)&&(!shadow) ) || ( (loa==SDCFG)&&(!shadow) ) || ( (loa==SDDAT)&&(!shadow) ) ||

		    ( (loa==ATMF7)&&shadow ) || ( (loa==ATM77)&&shadow ) ||

		    ( loa==ZXEVBF )
		  )



			porthit = 1'b1;
		else
			porthit = 1'b0;
	end

	always @*
	begin
		if( ((loa==PORTFD) && (a[15:14]==2'b11)) || // 0xFFFD ports
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





	// dout data
	always @*
	begin
		case( loa )
		PORTFE:
			dout = { 1'b1, 1'b0/*tape_in*/, 1'b0, keys_in };


		NIDE10,NIDE30,NIDE50,NIDE70,NIDE90,NIDEB0,NIDED0,NIDEF0,NIDEC8:
			dout = idein[7:0];
		NIDE11:
			dout = idehiin;


		//PORTFD:

		VGSYS:
			dout = { vg_intrq, vg_drq, 6'b111111 };

		KJOY:
			dout = {3'b000, kj_in};
		KMOUSE:
			dout = mus_in;

		SDCFG:
			dout = 8'h00; // always SD inserted, SD is in R/W mode // FIXME!FIXME!FIXME!FIXME!FIXME!
		SDDAT:
			dout = sd_dataout;


		PORTF7: begin
			if( !a[14] && gluclock_on ) // $BFF7 - data i/o
				dout = wait_read;
			else // any other $xxF7 port
				dout = 8'hFF;
		end


		default:
			dout = 8'hFF;
		endcase
	end



	assign portfe_wr    = ( (loa==PORTFE) && port_wr);
	assign portfd_wr    = ( (loa==PORTFD) && port_wr);
	assign portf7_wr    = ( (loa==PORTF7) && port_wr && (!shadow) );
	assign portf7_rd    = ( (loa==PORTF7) && port_rd && (!shadow) );

	assign ideout_hi_wr = ( (loa==NIDE11) && port_wr);
	assign idein_lo_rd  = ( (loa==NIDE10) && port_rd);

	assign vg_wrFF = ( ( (loa==VGSYS)&&shadow ) && port_wr);


	//port FE beep/border bit
	always @(posedge zclk)
	begin
		if( portfe_wr )
		begin
			beep <= din[4];
			border <= din[2:0];
		end
	end


	// IDE ports

	always @*
		ideout[7:0] = din;

	always @(posedge zclk)
	begin
		if( ideout_hi_wr )
			ideout[15:8] <= din;

		if( idein_lo_rd )
			idehiin <= idein[15:8];
	end

	always @*
		case( loa )
		NIDE10,NIDE30,NIDE50,NIDE70,NIDE90,NIDEB0,NIDED0,NIDEF0,NIDEC8: ide_ports = 1'b1;
		default: ide_ports = 1'b0;
		endcase

	assign ide_a = a[7:5];
	// trying to fix old WD drives...
//	assign ide_cs0_n = iorq_n | (rd_n&wr_n) | (~ide_ports) | (~(loa!=NIDEC8));
//	assign ide_cs1_n = iorq_n | (rd_n&wr_n) | (~ide_ports) | (~(loa==NIDEC8));
	assign ide_cs0_n = (~ide_ports) | (~(loa!=NIDEC8));
	assign ide_cs1_n = (~ide_ports) | (~(loa==NIDEC8));
	// fix ends...
	assign ide_rd_n = iorq_n | rd_n | (~ide_ports);
	assign ide_wr_n = iorq_n | wr_n | (~ide_ports);
	assign idedataout = ide_rd_n;


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

	always @(posedge zclk)
	begin
		if( rstsync2 )
			p7ffd_rom_int <= rstrom[0];
		else if( (a[15]==1'b0) && portfd_wr && (!block7ffd) )
			p7ffd_rom_int <= din[4];
	end

	assign block7ffd=p7ffd_int[5] & block1m;


	// EFF7 port
	always @(posedge zclk, negedge rst_n)
	begin
		if( !rst_n )
			peff7_int <= 8'h00;
		else if( !a[12] && portf7_wr )
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

	assign gluclock_on = peff7_int[7];

	always @(posedge zclk)
	begin
		if( gluclock_on && portf7_wr ) // gluclocks on
		begin
			if( !a[13] ) // $DFF7 - addr reg
				gluclock_addr <= din;

			// write to waiting register is not here - in separate section managing wait_write
		end
	end






	// write to wait registers
	always @(posedge zclk)
	begin
		// gluclocks
		if( gluclock_on && portf7_wr && !a[14] ) // $BFF7 - data reg
			wait_write <= din;
	end

	// wait from wait registers
	//
	// ACHTUNG!!!! here portxx_wr are ON Z80 CLOCK! logic must change when moving to fclk strobes
	//
	assign wait_start_gluclock = ( (!shadow) && gluclock_on && !a[14] && (portf7_rd || portf7_wr) ); // $BFF7 - gluclock r/w

	always @(posedge zclk) // wait rnw - only meanful during wait
	begin
		if( port_wr )
			wait_rnw <= 1'b0;

		if( port_rd )
			wait_rnw <= 1'b1;
	end





	// VG93 control
	assign vg_cs_n =  (~shadow) | iorq_n | (rd_n & wr_n) | ( ~((loa==VGCOM)|(loa==VGTRK)|(loa==VGSEC)|(loa==VGDAT)) );



/*  DOS moved to zdos.v !
`ifdef SIMULATE
	initial
	begin
		dos <= 1'b0;
	end
`endif

	always @(posedge zclk)
	begin
		if( rstsync2 )
			dos <= ~rstrom[1];
		else if( (!mreq_n) && (!m1_n) )
		begin
			if( (a[15:8]==8'h3D) && p7ffd[4] )
				dos <= 1'b1;
			else if( a[15:14]!=2'b00 )
				dos <= 1'b0;
		end
	end
*/

// reset rom selection

	always @(posedge zclk)
	begin
		rstsync1<=~rst_n;
		rstsync2<=rstsync1;
	end




// SD card (z-control¸r compatible)

	wire sdcfg_wr,sddat_wr,sddat_rd;

	assign sdcfg_wr = ( (loa==SDCFG) && port_wr && (!shadow) );
	assign sddat_wr = ( (loa==SDDAT) && port_wr && (!shadow) );
	assign sddat_rd = ( (loa==SDDAT) && port_rd && (!shadow) );

	// SDCFG write - sdcs_n control
	always @(posedge zclk, negedge rst_n)
	begin
		if( !rst_n )
			sdcs_n <= 1'b1;
		else // posedge zclk
			if( sdcfg_wr )
				sdcs_n <= din[1];
	end


	// start signal for SPI module with resyncing to fclk

	reg sd_start_toggle;
	reg [2:0] sd_stgl;

	// Z80 clock
	always @(posedge zclk)
		if( sddat_wr || sddat_rd )
			sd_start_toggle <= ~sd_start_toggle;

	// FPGA clock
	always @(posedge fclk)
		sd_stgl[2:0] <= { sd_stgl[1:0], sd_start_toggle };

	assign sd_start = ( sd_stgl[1] != sd_stgl[2] );


	// data for SPI module
	assign sd_datain = wr_n ? 8'hFF : din;







/////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////
	// ATM ports //
	///////////////

	wire atm77_wr_fclk;
	wire zxevbf_wr_fclk;

	assign atmF7_wr_fclk = ( (loa==ATMF7) && shadow && port_wr_fclk );
	assign atm77_wr_fclk = ( (loa==ATM77) && shadow && port_wr_fclk );

	assign zxevbf_wr_fclk = ( (loa==ZXEVBF) && port_wr_fclk );


	// port BF write
	//
	always @(posedge fclk, negedge rst_n)
	if( !rst_n )
	begin
		shadow_en_reg = 1'b0;
		romrw_en_reg  = 1'b0;
	end
	else if( zxevbf_wr_fclk )
	begin
		shadow_en_reg <= din[0];
		romrw_en_reg  <= din[1];
	end

	assign romrw_en = romrw_en_reg;



	// port xx77 write
	always @(posedge fclk, negedge rst_n)
	if( !rst_n )
	begin
		atm_scr_mode = 3'b011;
		atm_turbo    = 1'b1;
		atm_pen      = 1'b0; // UNLIKE ATM - reset with normal ROMs! (TEMPORARY???)
		atm_cpm_n    = 1'b1; // no permanent dos
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



endmodule

