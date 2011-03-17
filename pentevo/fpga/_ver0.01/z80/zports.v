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


	input [4:0] keyout,

	output reg [2:0] border,
	output reg beep,

	output reg dos,


	output ay_bdir,
	output ay_bc1,

	output [7:0] p7ffd,
	output [7:0] peff7,

	input [1:0] rstrom,

	output vg_cs_n,
	input vg_intrq,vg_drq, // from vg93 module - drq + irq read
	output vg_wrFF,        // write strobe of #FF port

	output reg sdcs_n,
	output sd_start,
	output [7:0] sd_datain,
	input [7:0] sd_dataout

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




	assign loa=a[7:0];

	always @*
	begin
		if( (loa==PORTFE) || (loa==PORTFD) || (loa==PORTF7) || (loa==NIDE10) || (loa==NIDE11) || (loa==NIDE30) ||
		    (loa==NIDE50) || (loa==NIDE70) || (loa==NIDE90) || (loa==NIDEB0) || (loa==NIDED0) || (loa==NIDEF0) ||
		    (loa==NIDEC8) ||

		    ( (loa==VGCOM)&&dos ) || ( (loa==VGTRK)&&dos ) || ( (loa==VGSEC)&&dos ) || ( (loa==VGDAT)&&dos ) ||
		    ( (loa==VGSYS)&&dos ) || ( (loa==KJOY)&&(!dos) ) ||

		    (loa==KMOUSE) || (loa==SDCFG) || (loa==SDDAT) )

			porthit = 1'b1;
		else
			porthit = 1'b0;
	end

	always @*
	begin
		if( ((loa==PORTFD) && (a[15:14]==2'b11)) || // 0xFFFD ports
		    (( (loa==VGCOM)&&dos ) || ( (loa==VGTRK)&&dos ) || ( (loa==VGSEC)&&dos ) || ( (loa==VGDAT)&&dos )) ) // vg93 ports
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
		PORTFE:
			dout = { 1'b1, 1'b0/*tape_in*/, 1'b0, keyout };


		NIDE10,NIDE30,NIDE50,NIDE70,NIDE90,NIDEB0,NIDED0,NIDEF0,NIDEC8:
			dout = idein[7:0];
		NIDE11:
			dout = idehiin;


		//PORTFD:

		VGSYS:
			dout = { vg_intrq, vg_drq, 6'b111111 };

		KJOY:
			dout = 8'h00;
		KMOUSE:
			dout = 8'hFF;

		SDCFG:
			dout = 8'h00; // always SD inserted, SD is on R/W mode // FIXME!FIXME!FIXME!FIXME!FIXME!
		SDDAT:
			dout = sd_dataout;


		default:
			dout = 8'hFF;
		endcase
	end



	assign portfe_wr    = ( (loa==PORTFE) && port_wr);
	assign portfd_wr    = ( (loa==PORTFD) && port_wr);
	assign portf7_wr    = ( (loa==PORTF7) && port_wr);

	assign ideout_hi_wr = ( (loa==NIDE11) && port_wr);
	assign idein_lo_rd  = ( (loa==NIDE10) && port_rd);

	assign vg_wrFF = ( ( (loa==VGSYS)&&dos ) && port_wr);


	//port FE beep/border bit
	always @(posedge clk)
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
	assign ide_cs0_n = iorq_n | (rd_n&wr_n) | (~ide_ports) | (~(loa!=NIDEC8));
	assign ide_cs1_n = iorq_n | (rd_n&wr_n) | (~ide_ports) | (~(loa==NIDEC8));
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
	always @(posedge clk, negedge rst_n)
	begin
		if( !rst_n )
			peff7_int <= 8'h00;
		else if( (a[15:8]==8'hEF) && portf7_wr && (!block1m) )
			peff7_int <= din; // 4 - turbooff, 0 - p16c on, 2 - block1meg
	end
	assign block1m = peff7_int[2];

	assign p7ffd = block7ffd ? { 8'b00010000 } : { (block1m ? 3'b0 : p7ffd_int[7:5]),p7ffd_rom_int,p7ffd_int[3:0]};

	// video modes + turbo are always modifiable. page0 ram is being blocked off
	assign peff7 = block1m ? { 2'b00, peff7_int[5], peff7_int[4], 3'b000, peff7_int[0] } : peff7_int;



	// VG93 control
	assign vg_cs_n =  (~dos) | iorq_n | (rd_n & wr_n) | ( ~((loa==VGCOM)|(loa==VGTRK)|(loa==VGSEC)|(loa==VGDAT)) );



	// dos on-off

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
