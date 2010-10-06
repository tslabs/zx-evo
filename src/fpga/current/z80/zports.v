`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
//Modified by TS-Labs inc.
//
//Ports handling
//

module zports(

	input clk,   // z80 clock
	input fclk,  // global FPGA clock
	input rst_n, // system reset

	input      [7:0] din,
	output reg [7:0] dout,
	output dataout,
	input [15:0] a,

	input iorq_n,
	input mreq_n,
	input m1_n,
	input rd_n,
	input wr_n,

	output reg porthit, // when internal port hit occurs, this is 1, else 0; used for iorq1_n iorq2_n on zxbus

	output reg [15:0] ideout,
	input      [15:0] idein,
	output     idedataout, // IDE must IN data from IDE device when idedataout=0, else it OUTs
	output [2:0] ide_a,
	output ide_cs0_n,
	output ide_cs1_n,
	output ide_rd_n,
	output ide_wr_n,


	input [4:0] keys_in, // keys (port FE)
	input [7:0] mus_in,  // mouse (xxDF)
	input [4:0] kj_in,

	output reg [5:0] border,
	
	output reg dos,

	output ay_bdir,
	output ay_bc1,

	output reg [7:0] psd0,psd1,psd2,psd3,

	output [7:0] p7ffd,
	output [7:0] peff7,

	input [1:0] rstrom,

	output vg_cs_n,
	input vg_intrq,vg_drq, // from vg93 module - drq + irq read
	output vg_wrFF,        // write strobe of #FF port

	output reg sdcs_n,
	output sd_start,
	output [7:0] sd_datain,
	input [7:0] sd_dataout,


	output reg  [ 7:0] gluclock_addr,

	output wire        wait_start_gluclock, // begin wait from some ports
	output reg         wait_rnw,   // whether it was read(=1) or write(=0)
	output reg  [ 7:0] wait_write,
	input  wire [ 7:0] wait_read,
	
	//XT
	output reg [7:0] vcfg
	
);


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

	localparam SD0	  = 8'h0F;
	localparam SD1	  = 8'h1F;
	localparam SD2	  = 8'h4F;
	localparam SD3	  = 8'h5F;
	localparam CVX1   = 8'hFB;
	localparam CVX2   = 8'hDD;
	
	localparam XT 	  = 16'h55FF;
	localparam XTRD	  = 8'hEF;

	localparam VGCOM  = 8'h1F;
	localparam VGTRK  = 8'h3F;
	localparam VGSEC  = 8'h5F;
	localparam VGDAT  = 8'h7F;
	localparam VGSYS  = 8'hFF;

	localparam KJOY   = 8'h1F;
	localparam KMOUSE = 8'hDF;

	localparam SDCFG  = 8'h77;
	localparam SDDAT  = 8'h57;

	//PortXT Regs
	localparam xborder  = 8'h00;
	localparam vconfig  = 8'h08;

	reg rstsync1,rstsync2;

	reg xt_en, xt_cl;
	localparam xt_msb = 8;		//Reduce it to actual bitwidth used!!!
	reg [xt_msb-1:0] xt_addr;

	reg external_port;

	reg port_wr;
	reg port_rd;

    reg iowr_reg;
    reg iord_reg;

	wire [7:0] loa;


	wire portfe_wr;

	wire ideout_hi_wr;
	wire idein_lo_rd;
	reg [7:0] idehiin;
	reg ide_ports; // ide ports selected

	reg pre_bc1,pre_bdir;

	wire gluclock_on;

	assign loa=a[7:0];
	
	always @*
	begin
		if( (loa==PORTFE) || (loa==PORTFD) || (loa==PORTF7) || (loa==NIDE10) || (loa==NIDE11) || (loa==NIDE30) ||
		    (loa==NIDE50) || (loa==NIDE70) || (loa==NIDE90) || (loa==NIDEB0) || (loa==NIDED0) || (loa==NIDEF0) ||
		    (loa==NIDEC8) ||

		    ( (loa==VGCOM)&&dos ) || ( (loa==VGTRK)&&dos ) || ( (loa==VGSEC)&&dos ) || ( (loa==VGDAT)&&dos ) ||
		    ( (loa==VGSYS)&&dos ) || ( (loa==KJOY)&&(!dos) ) ||

		    ( (loa==SD0)&&(!dos) ) || ( (loa==SD1)&&(!dos) ) || ( (loa==SD2)&&(!dos) ) || ( (loa==SD3)&&(!dos) ) ||
			(loa==CVX1) || (loa==CVX2) || 
			
			(loa==KMOUSE) || (loa==SDCFG) || (loa==SDDAT) ||
			
			(loa == XTRD) || (a[15:0]==XT) )

			porthit = 1'b1;
		else
			porthit = 1'b0;
	end

	always @*
	begin
		if( ((loa==PORTFD) && (a[15:14]==2'b11)) || // 0xFFFD ports
		    (( (loa==VGCOM) && dos ) || ( (loa==VGTRK) && dos ) || ( (loa==VGSEC) && dos ) || ( (loa==VGDAT) && dos )) ) // vg93 ports
			external_port = 1'b1;
		else
			external_port = 1'b0;
	end

	assign dataout = porthit & (~iorq_n) & (~rd_n) & (~external_port);




	always @(posedge clk)
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


		// dout data
	always @*
	begin
		case( loa )

		//XT Regs are read here !!!
		XTRD:
		begin
		case (xt_addr)
		xborder:
			dout = border;
		endcase
		end

		PORTFE:
			dout = { 1'b1, 1'b0/*tape_in*/, 1'b0, keys_in };

		NIDE10,NIDE30,NIDE50,NIDE70,NIDE90,NIDEB0,NIDED0,NIDEF0,NIDEC8:
			dout = idein[7:0];
		NIDE11:
			dout = idehiin;

		VGSYS:
			dout = { vg_intrq, vg_drq, 6'b111111 };

		KJOY:
			dout = {3'b000, kj_in};
		KMOUSE:
			dout = mus_in;

		SDCFG:
			dout = 8'h00; // always SD inserted, SD is on R/W mode // FIXME!FIXME!FIXME!FIXME!FIXME!
		SDDAT:
			dout = sd_dataout;


		PORTF7:
			begin
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
	assign portf7_wr    = ( (loa==PORTF7) && port_wr);
	assign portf7_rd    = ( (loa==PORTF7) && port_rd);
	assign portcv_wr    = ( ((loa==CVX1) || (loa==CVX2)) && port_wr);
	assign portsd_wr    = ( ((loa==SD0) || (loa==SD1) || (loa==SD2) || (loa==SD3)) && port_wr && (!dos));
	assign portxt_wr    = ( (a[15:0]==XT) && port_wr );
//	assign portxt_rd    = ( (loa ==XT) && port_rd );

	assign ideout_hi_wr = ( (loa==NIDE11) && port_wr);
	assign idein_lo_rd  = ( (loa==NIDE10) && port_rd);

	assign vg_wrFF = ( ( (loa==VGSYS)&&dos ) && port_wr);



	// IDE ports

	always @*
		ideout[7:0] = din;

	always @(posedge clk)
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
	reg [7:0] p7ffd_int;
	reg p7ffd_rom_int;
	wire block7ffd;
	wire block1m;

	always @(posedge clk, negedge rst_n)
	begin
		if( !rst_n )
			p7ffd_int <= 7'h00;
		else if( (a[15]==1'b0) && portfd_wr && (!block7ffd) )
			p7ffd_int <= din; // 2..0 - page, 3 - screen, 4 - rom, 5 - block48k, 6..7 -
	end

	always @(posedge clk)
	begin
		if( rstsync2 )
			p7ffd_rom_int <= rstrom[0];
		else if( (a[15]==1'b0) && portfd_wr && (!block7ffd) )
			p7ffd_rom_int <= din[4];
	end


	assign block7ffd=p7ffd_int[5] & block1m;

	// EFF7 port
	reg [7:0] peff7_int;

	always @(posedge clk, negedge rst_n)
	begin
		if( !rst_n )
			peff7_int <= 8'h00;
//		lvd's style
//		else if( (a[15:8]==8'hEF) && portf7_wr && (!block1m) )
//		koe's style
		else if( /*(a[15:8]==8'hEF)*/ !a[12] && portf7_wr )
			peff7_int <= din; // 4 - turbooff, 0 - p16c on, 2 - block1meg
	end
	assign block1m = peff7_int[2];

	assign p7ffd = { (block1m ? 3'b0 : p7ffd_int[7:5]),p7ffd_rom_int,p7ffd_int[3:0]};

	assign peff7 = block1m ? { peff7_int[7], 1'b0, peff7_int[5], peff7_int[4], 3'b000, peff7_int[0] } : peff7_int;


	// gluclock ports (bit7:eff7 is above)

	assign gluclock_on = peff7_int[7];

	always @(posedge clk)
	begin
		if( gluclock_on && portf7_wr ) // gluclocks on
		begin
			if( !a[13] ) // $DFF7 - addr reg
				gluclock_addr <= din;

			// write to waiting register is not here - in separate section managing wait_write
		end
	end

	
			
	//#55FF - portXT

	//#55FF lock managing
	always @(posedge clk, negedge rst_n)
	begin
		if( !rst_n )
			begin
			xt_en <= 1'b0;		//on ~RES portXT <= disabled
			end

		else if( portxt_wr && (din[7:0]==8'hAA ) && (!xt_en) )	//if out(#55ff),#aa
			begin
			xt_en <= 1'b1;		//portXT <= enabled
			xt_cl <= 1'b0;		//cycle <= AddrLatch
			end

		else if( portxt_wr && xt_en && (!xt_cl) )				//if out(#55ff) when portXT enabled and cycle = AddrLatch
			begin
			xt_addr <= din[7:0];
			xt_cl <= 1'b1;		// cycle <= DataLatch
			end

		else if( ( portxt_wr && xt_en && xt_cl ) || (port_wr && !(a[15:0]==XT)))	//if last cycle or write to other port then XT disable
			begin
			xt_en <= 1'b0;			//portXT disabled
			end
	end

	//#55FF Write to XT Regs and allied ports
	always @(posedge clk, negedge rst_n)
	
	if( !rst_n )
		begin
			vcfg <= 8'b0;
		end
		else
	
	begin
		if	((portxt_wr && xt_en && xt_cl) ||
			portfe_wr || portcv_wr || portsd_wr)
		begin

		//port FE beep/border bit
		if (portfe_wr)
		begin
			psd0 <= {din[4],din[4],din[4],din[4],din[4],din[4],din[4],din[4]};
			psd1 <= {din[4],din[4],din[4],din[4],din[4],din[4],din[4],din[4]};
			psd2 <= {din[4],din[4],din[4],din[4],din[4],din[4],din[4],din[4]};
			psd3 <= {din[4],din[4],din[4],din[4],din[4],din[4],din[4],din[4]};
			border <= {din[1],1'b0,din[2],1'b0,din[0],1'b0};
		end

		//FB,DD,0F,1F,4F,5F - Covox/Soundrive ports
		if (portcv_wr || portsd_wr)
		case( loa )
			SD0:
				psd0 <= din[7:0];
			SD1:
				psd1 <= din[7:0];
			SD2:
				psd2 <= din[7:0];
			SD3:
				psd3 <= din[7:0];
			default:
			begin
				psd0 <= din[7:0];
				psd1 <= din[7:0];
				psd2 <= din[7:0];
				psd3 <= din[7:0];
			end
		endcase
		
		if	(portxt_wr && xt_en && xt_cl)
		case (xt_addr)								//XT Regs are written here !!!
		xborder:
			border <= din[5:0];
		vconfig:
			vcfg <= din[7:0];
		endcase
		end

	end


	
	// write to wait registers
	always @(posedge clk)
	begin
		// gluclocks
		if( gluclock_on && portf7_wr && !a[14] ) // $BFF7 - data reg
			wait_write <= din;
	end

	// write from wait registers
	assign wait_start_gluclock = ( gluclock_on && !a[14] && (portf7_rd || portf7_wr) ); // $BFF7 - gluclock r/w

	always @(posedge clk) // wait rnw - only meanful during wait
	begin
		if( port_wr )
			wait_rnw <= 1'b0;

		if( port_rd )
			wait_rnw <= 1'b1;
	end

	

	// VG93 control
	assign vg_cs_n =  (~dos) | iorq_n | (rd_n & wr_n) | ( ~((loa==VGCOM)|(loa==VGTRK)|(loa==VGSEC)|(loa==VGDAT)) );


	// dos on-off

`ifdef SIMULATE
	initial
	begin
		dos <= 1'b0;
	end
`endif

	always @(posedge clk)
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


// reset rom selection

	always @(posedge clk)
	begin
		rstsync1<=~rst_n;
		rstsync2<=rstsync1;
	end


// SD card (z-control¸r compatible)

	wire sdcfg_wr,sddat_wr,sddat_rd;

	assign sdcfg_wr = ( (loa==SDCFG) && port_wr);
	assign sddat_wr = ( (loa==SDDAT) && port_wr);
	assign sddat_rd = ( (loa==SDDAT) && port_rd);

	// SDCFG write - sdcs_n control
	always @(posedge clk, negedge rst_n)
	begin
		if( !rst_n )
			sdcs_n <= 1'b1;
		else // posedge clk
			if( sdcfg_wr )
				sdcs_n <= din[1];
	end


	// start signal for SPI module with resyncing to fclk

	reg sd_start_toggle;
	reg [2:0] sd_stgl;

	// Z80 clock
	always @(posedge clk)
		if( sddat_wr || sddat_rd )
			sd_start_toggle <= ~sd_start_toggle;

	// FPGA clock
	always @(posedge fclk)
		sd_stgl[2:0] <= { sd_stgl[1:0], sd_start_toggle };

	assign sd_start = ( sd_stgl[1] != sd_stgl[2] );


	// data for SPI module
	assign sd_datain = wr_n ? 8'hFF : din;



endmodule
