// PentEvo project (c) NedoPC 2008-2010
//
// most of pentevo ports are here

`include "../include/tune.v"

module zports(

	input  wire        zclk,   // z80 clock
	input  wire        fclk,

	input  wire        zpos,
	input  wire        zneg,


	input  wire [ 7:0] din,
	output reg  [ 7:0] dout,
	output wire        dataout,
	input  wire [15:0] a,

	input  wire        rst_n, // system reset
	input  wire        iorq_n,
	input  wire        mreq_n,
	input  wire        m1_n,
	input  wire        rd_n,
	input  wire        wr_n,

	input  wire        iorq,
	input  wire        iorq_s,
	input  wire        mreq,
	input  wire        m1,
	input  wire        iord,
	input  wire        iowr,
	input  wire        iowr_s,
	input  wire        iorw,
	input  wire        rd,
	input  wire        wr,
	input  wire        rdwr,

	output wire        porthit, // when internal port hit occurs, this is 1, else 0; used for iorq1_n iorq2_n on zxbus
	output wire        external_port, // asserts for AY and VG93 accesses

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


// eXTension ports
    output wire zborder_wr  ,
    output wire border_wr   ,
    output wire zvpage_wr	,
    output wire vpage_wr	,
    output wire vconf_wr	,
    output wire gx_offsl_wr	,
    output wire gx_offsh_wr	,
    output wire gy_offsl_wr	,
    output wire gy_offsh_wr	,
    output wire t0x_offsl_wr,
    output wire t0x_offsh_wr,
    output wire t0y_offsl_wr,
    output wire t0y_offsh_wr,
    output wire t1x_offsl_wr,
    output wire t1x_offsh_wr,
    output wire t1y_offsl_wr,
    output wire t1y_offsh_wr,
    output wire tsconf_wr	,
    output wire palsel_wr	,
    output wire tmpage_wr	,
    output wire t0gpage_wr	,
    output wire t1gpage_wr	,
    output wire sgpage_wr	,
    output wire hint_beg_wr ,
    output wire vint_begl_wr,
    output wire vint_begh_wr,

	output wire [31:0] xt_page,

	output reg [4:0] fmaddr,

	output reg [7:0] sysconf,
	output reg [7:0] memconf,
	output reg [7:0] im2vect,
	output reg [3:0] fddvirt,

	output wire [8:0] dmaport_wr,
	input  wire       dma_act,

	input  wire        dos,
	input  wire        vdos,
	output  wire       vdos_on,
	output  wire       vdos_off,


	output wire        ay_bdir,
	output wire        ay_bc1,

	output wire [ 7:0] peff7,

	input  wire [ 1:0] rstrom,

	input  wire        tape_read,

	input  wire        vg_intrq,
	input  wire        vg_drq, // from vg93 module - drq + irq read
	output wire        vg_cs_n,
	output wire        vg_wrFF,
   	output reg  [1:0]  vg_a, // disk drive selection

	output reg         sdcs_n,
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

	output wire        romrw_en, // from port BF


	output wire        p7ffd_ram0_0, // d3.eff7
	output wire        p7ffd_1m_on,  // d2.eff7
	// output wire [ 5:0] p7ffd_page,   // full 1 meg page number


	output wire        covox_wr,
	output wire        beeper_wr

);

	// assign t0 = portfd_wr | portfe_wr;	//debug!!!


	reg rstsync1,rstsync2;

	localparam PORTFE = 8'hFE;
	localparam PORTXT = 8'hAF;
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

	localparam COMPORT = 8'hEF; // F8EF..FFEF - rs232 ports

	localparam COVOX   = 8'hFB;


	reg port_wr;
	reg port_rd;

	reg iowr_reg;
	reg iord_reg;

	reg port_wr_fclk;
	reg port_rd_fclk;
	// reg mem_wr_fclk;

	reg [1:0] iowr_reg_fclk,
	          iord_reg_fclk;

	reg [1:0] memwr_reg_fclk;


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

	wire gluclock_on;

	wire [7:0] loa=a[7:0];
	wire [7:0] hoa=a[15:8];

    assign porthit =
            (((loa==SDCFG) || (loa==SDDAT)) && (!dos || vdos))
         || (loa==PORTFE) || (loa==PORTXT) || (loa==PORTFD)
		 || (loa==NIDE10) || (loa==NIDE11) || (loa==NIDE30) || (loa==NIDE50) || (loa==NIDE70)
		 || (loa==NIDE90) || (loa==NIDEB0) || (loa==NIDED0) || (loa==NIDEF0) || (loa==NIDEC8)
		 || (loa==KMOUSE)
		 || (((loa==VGCOM) || (loa==VGTRK) || (loa==VGSEC) || (loa==VGDAT) || (loa==VGSYS)) && dos)
         || ((loa==KJOY) && !dos)
		 || ((loa==PORTF7) && !dos)
         || (loa==COMPORT)
		  ;


	assign external_port = ( ((loa==PORTFD) & a[15])    // AY
                         || (( (loa==VGCOM) && dos ) || ( (loa==VGTRK)&&dos ) || ( (loa==VGSEC)&&dos ) || ( (loa==VGDAT)&&dos ))
                            );

	assign dataout = porthit & iord & (~external_port);


	// zclk-synchronous strobes
	always @(posedge zclk)
	begin
		iowr_reg <= iowr;
		iord_reg <= iord;

		if (!iowr_reg && iorq && wr)
			port_wr <= 1'b1;
		else
			port_wr <= 1'b0;


		if (!iord_reg && iorq && rd)
			port_rd <= 1'b1;
		else
			port_rd <= 1'b0;
	end


	// fclk-synchronous stobes
	//
	always @(posedge fclk) if( zpos )
	begin
		iowr_reg_fclk[0] <= iowr;
		iord_reg_fclk[0] <= iord;
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

	// always @(posedge fclk)
		// mem_wr_fclk <= memwr_reg_fclk[0] && (!memwr_reg_fclk[1]);



	// dout data
	always @*
	begin
		case( loa )
		PORTFE:
			dout = { 1'b1, tape_read, 1'b0, keys_in };

		NIDE10,NIDE30,NIDE50,NIDE70,NIDE90,NIDEB0,NIDED0,NIDEF0,NIDEC8:
			dout = iderdeven;
		NIDE11:
			dout = iderdodd;

        PORTXT:
            begin
            case (hoa)
            XSTAT:
                dout = {dma_act, 7'b0};
            RAMPAGE+2, RAMPAGE+3:
                dout = rampage[hoa[1:0]];
            default:
                dout = 8'hFF;
            endcase
            end

		VGSYS:
			dout = { vg_intrq, vg_drq, 6'b111111 };

		KJOY:
			dout = {3'b000, kj_in};
		KMOUSE:
			dout = mus_in;

		SDCFG:
			dout = 8'h00; // always SD inserted, SD is in R/W mode
		SDDAT:
			dout = sd_dataout;


		PORTF7:
        begin
			if( !a[14] && (a[8]^dos) && gluclock_on ) // $BFF7 - data i/o
				dout = wait_read;
			else // any other $xxF7 port
				dout = 8'hFF;
		end

		COMPORT:
        begin
			dout = wait_read; // $F8EF..$FFEF
		end

		default:
			dout = 8'hFF;
		endcase
	end


	// F7 ports (like EFF7) are accessible in dos mode but at addresses like EEF7, DEF7, BEF7 so that
	// there are no conflicts in dos mode with ATM xFF7 and x7F7 ports
	wire portf7_wr    = ( (loa==PORTF7) && (a[8]==1'b1) && port_wr && (!dos) ) ||
	                      ( (loa==PORTF7) && (a[8]==1'b0) && port_wr &&   dos  ) ;

	wire portf7_rd    = ( (loa==PORTF7) && (a[8]==1'b1) && port_rd && (!dos) ) ||
	                      ( (loa==PORTF7) && (a[8]==1'b0) && port_rd &&   dos  ) ;

	wire comport_wr   = ( (loa==COMPORT) && port_wr);
	wire comport_rd   = ( (loa==COMPORT) && port_rd);

	wire portxt_wr    = ((loa==PORTXT) && iowr_s);
	wire portfe_wr    = ((loa==PORTFE) && iowr_s);
	wire portfd_wr    = ((loa==PORTFD) && iowr_s);


//eXTension port #nnAF

	localparam VCONF		= 8'h00;
	localparam VPAGE		= 8'h01;
	localparam GXOFFSL		= 8'h02;
	localparam GXOFFSH		= 8'h03;
	localparam GYOFFSL		= 8'h04;
	localparam GYOFFSH		= 8'h05;
	localparam TSCONF		= 8'h06;
	localparam PALSEL 		= 8'h07;
	localparam TMPAGE		= 8'h08;
	localparam T0GPAGE		= 8'h09;
	localparam T1GPAGE		= 8'h0A;
	localparam SGPAGE		= 8'h0B;
	localparam XBORDER		= 8'h0F;

	localparam T0XOFFSL		= 8'h40;
	localparam T0XOFFSH		= 8'h41;
	localparam T0YOFFSL		= 8'h42;
	localparam T0YOFFSH		= 8'h43;
	localparam T1XOFFSL		= 8'h44;
	localparam T1XOFFSH		= 8'h45;
	localparam T1YOFFSL		= 8'h46;
	localparam T1YOFFSH		= 8'h47;
    
	localparam RAMPAGE		= 8'h10;	// this covers #10-#13
	localparam NWRADDR		= 8'h14;
	localparam FMADDR		= 8'h15;
	localparam RAMPAGES		= 8'h16;	// this covers #16-#17
	localparam DMASADDRL	= 8'h1A;
	localparam DMASADDRH	= 8'h1B;
	localparam DMASADDRX	= 8'h1C;
	localparam DMADADDRL	= 8'h1D;
	localparam DMADADDRH	= 8'h1E;
	localparam DMADADDRX	= 8'h1F;

	localparam SYSCONF		= 8'h20;
	localparam MEMCONF		= 8'h21;
	localparam HSINT		= 8'h22;
	localparam VSINTL		= 8'h23;
	localparam VSINTH		= 8'h24;
	localparam IM2VECT		= 8'h25;
	localparam DMALEN		= 8'h26;
	localparam DMACTRL		= 8'h27;
	localparam DMANUM		= 8'h28;
	localparam FDDVIRT		= 8'h29;

	localparam XSTAT		= 8'h00;


	assign dmaport_wr[0] = portxt_wr & (hoa == DMASADDRL);
	assign dmaport_wr[1] = portxt_wr & (hoa == DMASADDRH);
	assign dmaport_wr[2] = portxt_wr & (hoa == DMASADDRX);
	assign dmaport_wr[3] = portxt_wr & (hoa == DMADADDRL);
	assign dmaport_wr[4] = portxt_wr & (hoa == DMADADDRH);
	assign dmaport_wr[5] = portxt_wr & (hoa == DMADADDRX);
	assign dmaport_wr[6] = portxt_wr & (hoa == DMALEN);
	assign dmaport_wr[7] = portxt_wr & (hoa == DMACTRL);
	assign dmaport_wr[8] = portxt_wr & (hoa == DMANUM);

	assign zborder_wr   = portfe_wr;
	assign border_wr    = (portxt_wr & (hoa == XBORDER));
    assign zvpage_wr	=  p7ffd_wr;
    assign vpage_wr	    = (portxt_wr & (hoa == VPAGE ));
    assign vconf_wr	    = (portxt_wr & (hoa == VCONF ));
    assign gx_offsl_wr	= (portxt_wr & (hoa == GXOFFSL));
    assign gx_offsh_wr	= (portxt_wr & (hoa == GXOFFSH));
    assign gy_offsl_wr	= (portxt_wr & (hoa == GYOFFSL));
    assign gy_offsh_wr	= (portxt_wr & (hoa == GYOFFSH));
    assign t0x_offsl_wr	= (portxt_wr & (hoa == T0XOFFSL));
    assign t0x_offsh_wr	= (portxt_wr & (hoa == T0XOFFSH));
    assign t0y_offsl_wr	= (portxt_wr & (hoa == T0YOFFSL));
    assign t0y_offsh_wr	= (portxt_wr & (hoa == T0YOFFSH));
    assign t1x_offsl_wr	= (portxt_wr & (hoa == T1XOFFSL));
    assign t1x_offsh_wr	= (portxt_wr & (hoa == T1XOFFSH));
    assign t1y_offsl_wr	= (portxt_wr & (hoa == T1YOFFSL));
    assign t1y_offsh_wr	= (portxt_wr & (hoa == T1YOFFSH));
    assign tsconf_wr	= (portxt_wr & (hoa == TSCONF));
    assign palsel_wr	= (portxt_wr & (hoa == PALSEL));
	assign tmpage_wr	= (portxt_wr & (hoa == TMPAGE));
	assign t0gpage_wr	= (portxt_wr & (hoa == T0GPAGE));
	assign t1gpage_wr	= (portxt_wr & (hoa == T1GPAGE));
	assign sgpage_wr	= (portxt_wr & (hoa == SGPAGE));
    assign hint_beg_wr  = (portxt_wr & (hoa == HSINT ));
    assign vint_begl_wr = (portxt_wr & (hoa == VSINTL));
    assign vint_begh_wr = (portxt_wr & (hoa == VSINTH));


	reg [7:0] rampage[0:3];
	assign xt_page = {rampage[3], rampage[2], rampage[1], rampage[0]};

    wire lock128 = (memconf[7:6] == 2'b01);
   	assign romrw_en = memconf[1];

	always @(posedge fclk)
		if (!rst_n)
		begin
			fmaddr[4] <= 1'b0;
			im2vect <= 8'hFF;
			fddvirt <= 4'b0;
			sysconf <= 8'h01;       // turbo 7 MHz
			memconf <= 8'h04;       // no map

			rampage[0] <= 8'h00;
			rampage[1] <= 8'h05;
			rampage[2] <= 8'h02;
			rampage[3] <= 8'h00;
		end

        else
       	if (p7ffd_wr)
        begin
            memconf[0] <= din[4];
            rampage[3] <= {3'b0, lock128 ? 2'b0 : din[7:6], din[2:0]};
        end

		else
		if (portxt_wr)
		begin
			if (hoa[7:2] == RAMPAGE[7:2])
            begin
				rampage[hoa[1:0]] <= din;
            end

			if (hoa == FMADDR)
				fmaddr <= din[4:0];

			if (hoa == SYSCONF)
				sysconf <= din;

			if (hoa == MEMCONF)
				memconf <= din;

			if (hoa == IM2VECT)
				im2vect <= din;

			if (hoa == FDDVIRT)
				fddvirt <= din[3:0];
		end


	// IDE ports
	// IDE physical ports (routed to IDE device)
	always @(loa)
		case( loa )
		NIDE10,NIDE30,NIDE50,NIDE70,NIDE90,NIDEB0,NIDED0,NIDEF0,NIDEC8: ide_ports = 1'b1;
		default: ide_ports = 1'b0;
		endcase


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


	// assign idedataout = ide_rd_n;
	assign idedataout = !ide_wr_n;



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
	wire ay_hit = (loa==PORTFD) & a[15] & iorq;
	assign ay_bc1  = ay_hit & a[14] & (rd|(wr));
	assign ay_bdir = ay_hit & (wr);


	// 7FFD port
	reg [7:0] p7ffd, peff7_int;
	wire block7ffd = p7ffd[5];
	wire block1m;
	wire p7ffd_wr = !a[15] && portfd_wr && !block7ffd;

	always @(posedge fclk)
	begin
		if (!rst_n)
		begin
			p7ffd <= 8'h00;
			// p7ffd_page <= 6'd0;
		end
		else
		begin
			if (p7ffd_wr)
			begin
				p7ffd <= din;
			end
		end
	end


	// always @(posedge zclk)
	// begin
		// if (rstsync2)
			// p7ffd_rom_int <= rstrom[0];
		// else if (p7ffd_wr)
			// p7ffd_rom_int <= din[4];
	// end


	// EFF7 port
	always @(posedge zclk)
	begin
		if( !rst_n )
			peff7_int <= 8'h00;
		else if( !a[12] && portf7_wr && (!dos) ) // EEF7 in dos mode is abandoned!
			peff7_int <= din; // 4 - turbooff, 0 - p16c on, 2 - block1m
	end
	assign block1m = peff7_int[2];

	assign peff7 = block1m ? (peff7_int & 8'b10110001) : peff7_int;


	assign p7ffd_1m_on     = ~peff7_int[2];
	assign p7ffd_ram0_0    = peff7_int[3];


	// gluclock ports (bit7:eff7 is above)
	assign gluclock_on = peff7_int[7] || dos; // in dos mode EEF7 is abandoned: instead, gluclock access
	                                             // is ON forever in dos mode.

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


	// VG93
    wire virt_vg = fddvirt[vg_a];

    wire vg_port = ((loa==VGCOM) | (loa==VGTRK) | (loa==VGSEC) | (loa==VGDAT)) & dos;
    wire vgsys_port = (loa==VGSYS) & dos;

	wire vgsys_wr_int = vgsys_port && port_wr;

    assign vg_cs_n = !(iorw & vg_port & !virt_vg);
    assign vg_wrFF = vgsys_wr_int & !virt_vg;

    assign vdos_on = iorq_s & rdwr & (vg_port | vgsys_port) & dos & !vdos & virt_vg;
    assign vdos_off = iorq_s & rdwr & vg_port & vdos & virt_vg;

    always @(posedge fclk)
        if (vgsys_wr_int)
            vg_a <= din[1:0];


// reset rom selection

	always @(posedge zclk)
	begin
		rstsync1<=~rst_n;
		rstsync2<=rstsync1;
	end


// SD card (Z-control¸r compatible)
	wire sdcfg_wr;
    wire sddat_wr;
    wire sddat_rd;

	assign sdcfg_wr = ((loa==SDCFG) && port_wr_fclk && (!dos || vdos));
	assign sddat_wr = ((loa==SDDAT) && port_wr_fclk && (!dos || vdos));
	assign sddat_rd = ((loa==SDDAT) && port_rd_fclk);

	// SDCFG write - sdcs_n control
	always @(posedge fclk)
	begin
		if (!rst_n)
			sdcs_n <= 1'b1;
		else if (sdcfg_wr)
			sdcs_n <= din[1];
	end


	// start signal for SPI module with resyncing to fclk
	assign sd_start = sddat_wr || sddat_rd;


	// data for SPI module
	assign sd_datain = wr_n ? 8'hFF : din;


	// covox/beeper writes
	assign beeper_wr = (loa==PORTFE) && iowr;
	assign covox_wr  = (loa==COVOX) && iowr;


endmodule
