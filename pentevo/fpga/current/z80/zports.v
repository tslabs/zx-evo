// PentEvo project (c) NedoPC 2008-2010

`include "../include/tune.v"

module zports(

	input  wire        zclk,    // z80 clock
	input  wire        clk,

	input  wire [ 7:0] din,
	output reg  [ 7:0] dout,
	output wire        dataout,
	input  wire [15:0] a,

	input  wire        rst,     // system reset
	input  wire        opfetch_s,

	input  wire        rd,
	input  wire        wr,
	input  wire        rdwr,

	input  wire        iorq,
	input  wire        iorq_s,
	input  wire        iorq_s2,
	input  wire        iord,
	input  wire        iord_s,
	input  wire        iowr,
	input  wire        iowr_s,
	input  wire        iorw,
	input  wire        iorw_s,

	output wire        porthit,       // when internal port hit occurs, this is 1, else 0; used for iorq1_n iorq2_n on zxbus
	output wire        external_port, // asserts for AY and VG93 accesses

    output wire     zborder_wr  ,
    output wire     border_wr   ,
    output wire     zvpage_wr	,
    output wire     vpage_wr	,
    output wire     vconf_wr	,
    output wire     gx_offsl_wr	,
    output wire     gx_offsh_wr	,
    output wire     gy_offsl_wr	,
    output wire     gy_offsh_wr	,
    output wire     t0x_offsl_wr,
    output wire     t0x_offsh_wr,
    output wire     t0y_offsl_wr,
    output wire     t0y_offsh_wr,
    output wire     t1x_offsl_wr,
    output wire     t1x_offsh_wr,
    output wire     t1y_offsl_wr,
    output wire     t1y_offsh_wr,
    output wire     tsconf_wr	,
    output wire     palsel_wr	,
    output wire     tmpage_wr	,
    output wire     t0gpage_wr	,
    output wire     t1gpage_wr	,
    output wire     sgpage_wr	,
    output wire     hint_beg_wr ,
    output wire     vint_begl_wr,
    output wire     vint_begh_wr,

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
	output wire        covox_wr,
	output wire        beeper_wr,

	input  wire [ 1:0] rstrom,
	input  wire        tape_read,

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

	input  wire        vg_intrq,
	input  wire        vg_drq,  // from vg93 module - drq + irq read
	output wire        vg_cs_n,
	output wire        vg_wrFF,
   	output reg  [1:0]  drive_sel,    // disk drive selection

	output reg         sdcs_n,
	output wire        sd_start,
	output wire [ 7:0] sd_datain,
	input  wire [ 7:0] sd_dataout,

	// WAIT-ports related
	output reg  [ 7:0] gluclock_addr,
	output reg  [ 2:0] comport_addr,
	output wire        wait_start_gluclock, // begin wait from some ports
	output wire        wait_start_comport,  //
	output reg         wait_rnw,   // whether it was read(=1) or write(=0)
	output reg  [ 7:0] wait_write,
	input  wire [ 7:0] wait_read

);


	localparam PORTFE = 8'hFE;
	localparam PORTFD = 8'hFD;
	localparam PORTXT = 8'hAF;
	localparam PORTF7 = 8'hF7;
	localparam COVOX  = 8'hFB;

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

	localparam VGCOM  = 8'h1F;
	localparam VGTRK  = 8'h3F;
	localparam VGSEC  = 8'h5F;
	localparam VGDAT  = 8'h7F;
	localparam VGSYS  = 8'hFF;

	localparam KJOY   = 8'h1F;
	localparam KMOUSE = 8'hDF;

	localparam SDCFG  = 8'h77;
	localparam SDDAT  = 8'h57;

	localparam COMPORT = 8'hEF;     // F8EF..FFEF - rs232 ports


	wire [7:0] loa=a[7:0];
	wire [7:0] hoa=a[15:8];

    assign porthit =
            ((loa==PORTFE) || (loa==PORTXT) || (loa==PORTFD) || (loa==COVOX))
		 || ((loa==PORTF7) && !dos)
         || ide_ports || ide_port11
		 || ((vg_port || vgsys_port) && dos)
         || ((loa==KJOY) && !dos)
		 || (loa==KMOUSE)
         || (((loa==SDCFG) || (loa==SDDAT)) && (!dos || vdos))
         || (loa==COMPORT);

	wire ide_ports = (loa==NIDE10) || (loa==NIDE30) || (loa==NIDE50) || (loa==NIDE70)
                  || (loa==NIDE90) || (loa==NIDEB0) || (loa==NIDED0) || (loa==NIDEF0) || (loa==NIDEC8); // ide ports selected
    wire ide_port10 = (loa==NIDE10);
    wire ide_port11 = (loa==NIDE11);

    wire vg_port = (loa==VGCOM) | (loa==VGTRK) | (loa==VGSEC) | (loa==VGDAT);
    wire vgsys_port = (loa==VGSYS);

	assign external_port = ((loa==PORTFD) & a[15])        // AY
                        || (((loa==VGCOM) || (loa==VGTRK) || (loa==VGSEC) || (loa==VGDAT)) && dos);

	assign dataout = porthit & iord & (~external_port);


// zclk-synchronous strobes
	reg iowr_reg;
	reg iord_reg;
	reg port_wr;
	reg port_rd;

	always @(posedge zclk)
	begin
		iowr_reg <= iowr;
		iord_reg <= iord;

		if (!iowr_reg && iowr)
			port_wr <= 1'b1;
		else
			port_wr <= 1'b0;

		if (!iord_reg && iord)
			port_rd <= 1'b1;
		else
			port_rd <= 1'b0;
	end


    wire no_ide = 1'b0;     // this should be compiled conditionally


// reading ports
	always @*
	begin
		case (loa)
		PORTFE:
			dout = {1'b1, tape_read, 1'b0, keys_in};

		NIDE10,NIDE30,NIDE50,NIDE70,NIDE90,NIDEB0,NIDED0,NIDEF0,NIDEC8:
			dout = iderdeven;
		NIDE11:
			dout = iderdodd;

        PORTXT:
            begin
            case (hoa)
            
            XSTAT:
                dout = {dma_act, 1'b0, pwr_up_reg, 1'b0, no_ide, 3'b0};
                
            DMASTAT:
                dout = {dma_act, 7'b0};
                
            RAMPAGE+2, RAMPAGE+3:
                dout = rampage[hoa[1:0]];
                
            default:
                dout = 8'hFF;

            endcase
            end

		VGSYS:
			dout = {vg_intrq, vg_drq, 6'b111111};

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
			if (!a[14] && (a[8] ^ dos) && gluclock_on) // $BFF7 - data i/o
				dout = wait_read;
				// dout = 8'h55;
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


// power-up
// This bit is loaded as 1 while FPGA is configured
// and automatically reset to 0 after STATUS port reading

    reg pwr_up = 1'b1;
    reg pwr_up_reg;
    always @(posedge clk)
        if (iord_s & (loa == PORTXT) & (hoa == XSTAT))
        begin
            pwr_up_reg <= pwr_up;
            pwr_up <= 1'b0;
        end


// writing ports

//#nnAF
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
	localparam DMASTAT		= 8'h27;

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

	assign beeper_wr = portfe_wr;
	wire portfe_wr = (loa==PORTFE) && iowr_s;
	assign covox_wr  = (loa==COVOX) && iowr_s;
	wire portxt_wr = (loa==PORTXT) && iowr_s;

	reg [7:0] rampage[0:3];
	assign xt_page = {rampage[3], rampage[2], rampage[1], rampage[0]};

    wire lock128 = memconf[7] ? m1_lock128 : memconf[6];

	reg m1_lock128;
    always @(posedge clk)
		if (opfetch_s)
			m1_lock128 <= !(din[7] ^ din[6]);


	always @(posedge clk)
		if (rst)
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
				rampage[hoa[1:0]] <= din;
			if (hoa == FMADDR)
				fmaddr <= din[4:0];
			if (hoa == SYSCONF)
				sysconf <= din;
				// sysconf <= 8'b01;
			if (hoa == MEMCONF)
				memconf <= din;
			if (hoa == IM2VECT)
				im2vect <= din;
			if (hoa == FDDVIRT)
				fddvirt <= din[3:0];
		end


// 7FFD port
	wire p7ffd_wr = !a[15] && (loa==PORTFD) && iowr_s && !lck48;

	reg lck48;
	always @(posedge clk)
		if (rst)
			lck48 <= 1'b0;
		else if (p7ffd_wr)
			lck48 <= din[5];


// AY control
	wire ay_hit = (loa==PORTFD) & a[15] & iorq;
	assign ay_bc1  = ay_hit & a[14] & rdwr;
	assign ay_bdir = ay_hit & wr;


// VG93
    wire virt_vg = fddvirt[drive_sel];

    assign vg_cs_n = !(iorw & vg_port & dos & !vdos & !virt_vg);
    assign vg_wrFF = wr & iorq_s2 & vgsys_port & dos & !vdos & !virt_vg;

    assign vdos_on  = rdwr & iorq_s2 & (vg_port | vgsys_port) & dos & !vdos & virt_vg;
    assign vdos_off = rdwr & iorq_s2 & vg_port & vdos;

    // write drive number
    always @(posedge clk)
        if (iowr_s & vgsys_port & dos)
            drive_sel <= din[1:0];


// SD card (Z-control¸r compatible)
	wire sdcfg_wr;
    wire sddat_wr;
    wire sddat_rd;

	assign sdcfg_wr = ((loa==SDCFG) && iowr_s && (!dos || vdos));
	assign sddat_wr = ((loa==SDDAT) && iowr_s && (!dos || vdos));
	assign sddat_rd = ((loa==SDDAT) && iord_s);

	// SDCFG write - sdcs_n control
	always @(posedge clk)
		if (rst)
			sdcs_n <= 1'b1;
		else if (sdcfg_wr)
			sdcs_n <= din[1];


	// start signal for SPI module with resyncing to fclk
	assign sd_start = sddat_wr || sddat_rd;


	// data for SPI module
	assign sd_datain = wr ? din : 8'hFF;


// xxF7
	wire portf7_wr = ((loa==PORTF7) && (a[8]==1'b1) && port_wr && (!dos || vdos));
	wire portf7_rd = ((loa==PORTF7) && (a[8]==1'b1) && port_rd && (!dos || vdos));


// EFF7 port
    reg [7:0] peff7;
	always @(posedge clk)
		if (rst)
			peff7 <= 8'h00;
		else if (!a[12] && portf7_wr && !dos)   // #EEF7 in dos is not accessible
			peff7 <= din;


// gluclock ports
	wire gluclock_on = peff7[7] || dos;        // in dos mode EEF7 is not accessible, gluclock access is ON in dos mode.

	always @(posedge zclk)
		if (gluclock_on && portf7_wr) // gluclocks on
			if( !a[13] ) // $DFF7 - addr reg
				gluclock_addr <= din;


// write to wait registers
	always @(posedge zclk)
	begin
		// gluclocks
		if (gluclock_on && portf7_wr && !a[14]) // $BFF7 - data reg
			wait_write <= din;
		// com ports
		else if (comport_wr) // $F8EF..$FFEF - comports
			wait_write <= din;
	end


// comports
	wire comport_wr   = ((loa==COMPORT) && port_wr);
	wire comport_rd   = ((loa==COMPORT) && port_rd);

	always @(posedge zclk)
		if (comport_wr || comport_rd)
			comport_addr <= a[10:8];


// wait from wait registers
	// ACHTUNG!!!! here portxx_wr are ON Z80 CLOCK! logic must change when moving to clk strobes
	assign wait_start_gluclock = (gluclock_on && !a[14] && (portf7_rd || portf7_wr)); // $BFF7 - gluclock r/w
	assign wait_start_comport = (comport_rd || comport_wr);

	always @(posedge zclk) // wait rnw - only meanful during wait
	begin
		if (port_wr)
			wait_rnw <= 1'b0;

		if (port_rd)
			wait_rnw <= 1'b1;
	end


// IDE ports
	// IDE physical ports (routed to IDE device)
	assign ide_a = a[7:5];
	assign ide_cs0_n = ~(ide_ports & (loa!=NIDEC8));
	assign ide_cs1_n = ~(ide_ports & (loa==NIDEC8));
	assign ide_rd_n  = ~(iord & ide_ports & !(ide_rd_latch && ide_port10));
	assign ide_wr_n  = ~(iowr & ide_ports & !(ide_port10 && !ide_wrlo_latch && !ide_wrhi_latch));
	                                          // do NOT generate IDE write, if neither of ide_wrhi|lo latches
	                                          // set and writing to NIDE10

	// control read & write triggers, which allow nemo-divide mod to work.
	// read trigger:
	reg ide_rd_trig;  // nemo-divide read trigger
	always @(posedge zclk)
	begin
		if (ide_port10 && port_rd && !ide_rd_trig)
			ide_rd_trig <= 1'b1;
		else if ((ide_ports || ide_port11) && (port_rd || port_wr))
			ide_rd_trig <= 1'b0;
	end

    // two triggers for write sequence
	reg ide_wrlo_trig,  ide_wrhi_trig;  // nemo-divide write triggers
	always @(posedge zclk)
	if ((ide_ports || ide_port11) && (port_rd || port_wr))
	begin
		if (ide_port11 && port_wr)
			ide_wrhi_trig <= 1'b1;
		else
			ide_wrhi_trig <= 1'b0;

		if (ide_port10 && port_wr && !ide_wrhi_trig && !ide_wrlo_trig)
			ide_wrlo_trig <= 1'b1;
		else
			ide_wrlo_trig <= 1'b0;
	end


	// normal read: #10(low), #11(high)
	// divide read: #10(low), #10(high)
	//
	// normal write: #11(high), #10(low)
	// divide write: #10(low),  #10(high)


	reg  [15:0] idewrreg; // write register, either low or high part is pre-written here,
	                      // while other part is out directly from Z80 bus
	always @(posedge zclk)
	begin
		if (port_wr && ide_port11)
			idewrreg[15:8] <= din;

		if (port_wr && ide_port10 && !ide_wrlo_trig)
			idewrreg[ 7:0] <= din;
	end


	wire idein_lo_rd  = port_rd && ide_port10 && (!ide_rd_trig);

	reg [7:0] idehiin; // IDE high part read register: low part is read directly to Z80 bus,
	                   // while high part is remembered here
	always @(posedge zclk)
	if (idein_lo_rd)
			idehiin <= idein[15:8];


	// generate read cycles for IDE as usual, except for reading #10
	// instead of #11 for high byte (nemo-divide). I use additional latch
	// since 'ide_rd_trig' clears during second Z80 IO read cycle to #10
	reg ide_rd_latch; // to save state of trigger during read cycle
	always @*
        if (!rd)
            ide_rd_latch <= ide_rd_trig;

	reg ide_wrlo_latch, ide_wrhi_latch; // save state during write cycles
	always @*
        if (!wr)
        begin
            ide_wrlo_latch <= ide_wrlo_trig; // same for write triggers
            ide_wrhi_latch <= ide_wrhi_trig;
        end


	// assign idedataout = ide_rd_n;    // IDE fix
	assign idedataout = !ide_wr_n;

	// data read by Z80 from IDE
	wire [7:0] iderdodd = idehiin[7:0];                                               // read data from "odd" port (#11)
	wire [7:0] iderdeven = (ide_rd_latch && ide_port10) ? idehiin[7:0] : idein[7:0];  // to control read data from "even" ide ports (all except #11)

	// data written to IDE from Z80
	assign ideout[15:8] = ide_wrhi_latch ? idewrreg[15:8] : din[ 7:0];
	assign ideout[ 7:0] = ide_wrlo_latch ? idewrreg[ 7:0] : din[ 7:0];


endmodule
