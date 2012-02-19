// PentEvo project (c) NedoPC 2008-2010
//
// most of pentevo ports are here

`include "../include/tune.v"

module zports(

	input  wire        zclk,   // z80 clock
	input  wire        fclk,
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
	// output wire        t0,	// debug!!!


	input  wire [ 4:0] keys_in, // keys (port FE)
	input  wire [ 7:0] mus_in,  // mouse (xxDF)
	input  wire [ 4:0] kj_in,

	
// eXTension ports
    output wire zborder_wr  ,
    output wire border_wr   ,
    output wire zvpage_wr	,
    output wire vpage_wr	,   
    output wire vconf_wr	,   
    output wire x_offsl_wr	,
    output wire x_offsh_wr	,
    output wire y_offsl_wr	,
    output wire y_offsh_wr	,
    output wire tsconf_wr	,
    output wire palsel_wr	,
    output wire tgpage_wr	,
    output wire hint_beg_wr ,
    output wire vint_begl_wr,
    output wire vint_begh_wr,
	
	output wire [31:0] xt_page,
	output reg [3:0] xt_override,
	
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


	output wire        atmF7_wr_fclk, // used in atm_pager.v


	output reg  [ 2:0] atm_scr_mode, // RG0..RG2 in docs
	output reg         atm_pen,      // pager_off in atm_pager.v, NOT inverted!!!
	output reg         atm_cpm_n,    // permanent dos on

	output wire        romrw_en, // from port BF


	output wire        p7ffd_ram0_0, // d3.eff7
	output wire        p7ffd_1m_on,  // d2.eff7
	// output wire [ 5:0] p7ffd_page,   // full 1 meg page number
	output wire        p7ffd_ROM,     // d4.7ffd


	output wire        covox_wr,
	output wire        beeper_wr,

	output wire        clr_nmi,

	output wire        fnt_wr,		// write to font_ram enabled

	// inputs from atm_pagers, to read back its config
	input  wire [63:0] pages,
	input  wire [ 7:0] ramnroms,
	input  wire [ 7:0] dos7ffds,


	// NMI generation
	output reg         set_nmi
);

	// assign t0 = portfd_wr | portfe_wr;	//debug!!!


	reg rstsync1,rstsync2;


	localparam PORTFE = 8'hFE;
	localparam PORTXT = 8'hAF;
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

	localparam COMPORT = 8'hEF; // F8EF..FFEF - rs232 ports

	localparam COVOX   = 8'hFB;

    
//eXTension port #nnAF

	localparam VCONF		= 8'h00;
	localparam VPAGE		= 8'h01;
	localparam XOFFSL		= 8'h02;
	localparam XOFFSH		= 8'h03;
	localparam YOFFSL		= 8'h04;
	localparam YOFFSH		= 8'h05;
	localparam TSCONF		= 8'h06;
	localparam PALSEL 		= 8'h07;
	localparam XBORDER		= 8'h0F;

	localparam RAMPAGE		= 8'h10;	// this uses #10-#13
	localparam NWRADDR		= 8'h14;
	localparam FMADDR		= 8'h15;
	localparam RAMPAGES		= 8'h16;	// this uses #16-#17
	localparam TGPAGE		= 8'h18;
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
	localparam XTOVERR  	= 8'h2A;

	localparam XSTAT		= 8'h00;

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



	reg  shadow_en_reg; //bit0.xxBF
	// reg   romrw_en_reg; //bit1.xxBF
	reg  fntw_en_reg; 	//bit2.xxBF

	wire shadow;



	reg [7:0] portbemux;



	reg [7:0] savport [3:0];





	assign shadow = dos || shadow_en_reg;






	wire [7:0] loa=a[7:0];
	wire [7:0] hoa=a[15:8];

    assign porthit = (
            (loa==PORTFE) || (loa==PORTXT) || (loa==PORTF6) || (loa==PORTFD) ||

		    (loa==NIDE10) || (loa==NIDE11) || (loa==NIDE30) || (loa==NIDE50) || (loa==NIDE70) ||
		    (loa==NIDE90) || (loa==NIDEB0) || (loa==NIDED0) || (loa==NIDEF0) || (loa==NIDEC8) ||

		    (loa==KMOUSE) ||

		    (((loa==VGCOM) || (loa==VGTRK) || (loa==VGSEC) || (loa==VGDAT) || (loa==VGSYS)) && shadow) ||
            
            ( (loa==KJOY)&&(!shadow) ) ||

    		( (loa==SAVPORT1)&&shadow ) || ( (loa==SAVPORT2)&&shadow ) || ( (loa==SAVPORT3)&&shadow ) || ( (loa==SAVPORT4)&&shadow ) ||

		    ( (loa==PORTF7)&&(!shadow) ) ||
            
            ( (loa==SDCFG)&&(!shadow) ) || ( (loa==SDDAT) ) ||

		    ( (loa==ATMF7)&&shadow ) || ( (loa==ATM77)&&shadow ) ||

		    ( loa==ZXEVBF ) || ( loa==ZXEVBE) || 
            
            ( loa==COMPORT )
		  );


	assign external_port = ( ((loa==PORTFD) & a[15]) || // AY
    
		   (( (loa==VGCOM)&&shadow ) || ( (loa==VGTRK)&&shadow ) || ( (loa==VGSEC)&&shadow ) || ( (loa==VGDAT)&&shadow ))
           
           );

	assign dataout = porthit & (~iorq_n) & (~rd_n) & (~external_port);


	// Z80 asynchronous signals
	wire iowr = ~(iorq_n | wr_n);
	wire iord = ~(iorq_n | rd_n);
    wire rdwr = (!rd_n | !wr_n);
    wire iorq = !iorq_n;
    

	// this is zclk-synchronous strobes
	always @(posedge zclk)
	begin
		iowr_reg <= iowr;
		iord_reg <= iord;

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
			// dout = { 4'b0000, set_nmi, fntw_en_reg, romrw_en_reg, shadow_en_reg };
			dout = { 4'b0000, set_nmi, fntw_en_reg, 1'b0, shadow_en_reg };
		end

`ifndef ANTIATM
		ZXEVBE: begin
			dout = portbemux;
		end
`endif


		default:
			dout = 8'hFF;
		endcase
	end



	wire portxt_wr    = ((loa==PORTXT) && iowr);
	wire portfe_wr    = ((loa==PORTFE) && iowr);
	wire portfd_wr    = ((loa==PORTFD) && iowr);

	// F7 ports (like EFF7) are accessible in shadow mode but at addresses like EEF7, DEF7, BEF7 so that
	// there are no conflicts in shadow mode with ATM xFF7 and x7F7 ports
	wire portf7_wr    = ( (loa==PORTF7) && (a[8]==1'b1) && port_wr && (!shadow) ) ||
	                      ( (loa==PORTF7) && (a[8]==1'b0) && port_wr &&   shadow  ) ;

	wire portf7_rd    = ( (loa==PORTF7) && (a[8]==1'b1) && port_rd && (!shadow) ) ||
	                      ( (loa==PORTF7) && (a[8]==1'b0) && port_rd &&   shadow  ) ;

	wire comport_wr   = ( (loa==COMPORT) && port_wr);
	wire comport_rd   = ( (loa==COMPORT) && port_rd);



//eXTension port #nnAF

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
    assign x_offsl_wr	= (portxt_wr & (hoa == XOFFSL));
    assign x_offsh_wr	= (portxt_wr & (hoa == XOFFSH));
    assign y_offsl_wr	= (portxt_wr & (hoa == YOFFSL));
    assign y_offsh_wr	= (portxt_wr & (hoa == YOFFSH));
    assign tsconf_wr	= (portxt_wr & (hoa == TSCONF));
    assign palsel_wr	= (portxt_wr & (hoa == PALSEL));
	assign tgpage_wr	= (portxt_wr & (hoa == TGPAGE));
    assign hint_beg_wr  = (portxt_wr & (hoa == HSINT ));
    assign vint_begl_wr = (portxt_wr & (hoa == VSINTL));
    assign vint_begh_wr = (portxt_wr & (hoa == VSINTH));

    
	reg [7:0] rampage[0:3];
	assign xt_page = {rampage[3], rampage[2], rampage[1], rampage[0]};
	
    wire lock128 = (memconf[7:6] == 2'b01);
   	assign romrw_en = memconf[1];
	assign p7ffd_ROM = memconf[0];
        
	always @(posedge zclk)
		if (!rst_n)
		begin
			fmaddr[4] <= 1'b0;
			im2vect <= 8'hFF;
			fddvirt <= 4'b0;
			sysconf <= 8'h01;       // turbo 7 MHz

`ifdef ANTIATM
			memconf <= 8'h04;       // ANTIATM
			xt_override <= 4'b1111;       // ANTIATM
`else
			memconf <= 8'h00;       // atm
			xt_override <= 4'b0;       // atm crotch
`endif
            
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
				xt_override[hoa[1:0]] <= 1'b1;	// if XT page write, correspondent override ON
            end
				
            if (hoa == XTOVERR)
				xt_override <= din[3:0];
                
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
	wire ay_hit = (loa==PORTFD) & a[15] & (~iorq_n);
	assign ay_bc1  = ay_hit & a[14] & ((~rd_n)|(~wr_n));
	assign ay_bdir = ay_hit & (~wr_n);
	

	// 7FFD port
	reg [7:0] p7ffd, peff7_int;
	wire block7ffd = p7ffd[5];
	wire block1m;
	wire p7ffd_wr = !a[15] && portfd_wr && !block7ffd;
	
	always @(posedge zclk)
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
		else if( !a[12] && portf7_wr && (!shadow) ) // EEF7 in shadow mode is abandoned!
			peff7_int <= din; // 4 - turbooff, 0 - p16c on, 2 - block1m
	end
	assign block1m = peff7_int[2];

	assign peff7 = block1m ? (peff7_int & 8'b10110001) : peff7_int;


	assign p7ffd_1m_on     = ~peff7_int[2];
	assign p7ffd_ram0_0    = peff7_int[3];




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
    wire virt_vg = fddvirt[vg_a];
    
    wire vg_port = ((loa==VGCOM) | (loa==VGTRK) | (loa==VGSEC) | (loa==VGDAT)) & shadow;
    wire vgsys_port = (loa==VGSYS) & shadow;
    
	wire vg_cs_n_int =  !(iorq & rdwr & vg_port);
    wire vgsys_cs_int = iorq & rdwr & vgsys_port;
	wire vgsys_wr_int = vgsys_port && port_wr;
	
    assign vg_cs_n = vg_cs_n_int | virt_vg;
    assign vg_wrFF = vgsys_wr_int & !virt_vg;
    
    assign vdos_on = (!vg_cs_n_int | vgsys_cs_int) & virt_vg;
    assign vdos_off = !vg_cs_n_int & vdos & virt_vg;
    // assign vdos_off = !vg_cs_n_int & vdos;

    always @(posedge fclk)
        if (vgsys_wr_int)
            vg_a <= din[1:0];


// reset rom selection

	always @(posedge zclk)
	begin
		rstsync1<=~rst_n;
		rstsync2<=rstsync1;
	end




// SD card (z-control¸r compatible)

	wire sdcfg_wr,sddat_wr,sddat_rd;

	assign sdcfg_wr = ( (loa==SDCFG) && port_wr_fclk && (!shadow) )                  ||
	                  ( (loa==SDDAT) && port_wr_fclk &&   shadow  && (a[15]==1'b1) ) ;

	assign sddat_wr = ( (loa==SDDAT) && port_wr_fclk && (!shadow) )                  ||
	                  ( (loa==SDDAT) && port_wr_fclk &&   shadow  && (a[15]==1'b0) ) ;

	assign sddat_rd = ( (loa==SDDAT) && port_rd_fclk              );

	// SDCFG write - sdcs_n control
	always @(posedge fclk)
	begin
		if( !rst_n )
			sdcs_n <= 1'b1;
		else // posedge zclk
			if( sdcfg_wr )
				sdcs_n <= din[1];
	end


	// start signal for SPI module with resyncing to fclk

	assign sd_start = sddat_wr || sddat_rd;


	// data for SPI module
	assign sd_datain = wr_n ? 8'hFF : din;







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
	always @(posedge fclk)
	if( !rst_n )
	begin
		shadow_en_reg <= 1'b0;
		// romrw_en_reg  <= 1'b0;
		fntw_en_reg   <= 1'b0;
		set_nmi       <= 1'b0;
	end
	else if( zxevbf_wr_fclk )
	begin
		shadow_en_reg <= din[0];
		// romrw_en_reg  <= din[1];
		fntw_en_reg   <= din[2];
		set_nmi       <= din[3];
	end


	// port xx77 write
	always @(posedge fclk)
	if( !rst_n )
	begin
        
`ifdef ANTIATM
		atm_pen <=   1'b0; // ANTIATM,
		atm_cpm_n <= 1'b1; // ANTIATM
`else
		atm_pen <=   1'b1; // no manager,
		atm_cpm_n <= 1'b0; // permanent dosen (shadow ports on)
`endif


	end
	else
    if( atm77_wr_fclk )
    begin
        atm_scr_mode <= din[2:0];
        atm_pen      <= ~a[8];
        atm_cpm_n    <=  a[9];
    end


	// port BE write
	assign clr_nmi = ( (loa==ZXEVBE) && port_wr_fclk );


	// covox/beeper writes
	assign beeper_wr = (loa==PORTFE) && iowr;
	assign covox_wr  = (loa==COVOX) && iowr;



	// font write enable
	assign fnt_wr = fntw_en_reg && mem_wr_fclk;



	// port BE read
`ifndef ANTIATM

	always @*
	case( a[11:8] )

	// 4'h0: portbemux = pages[ 7:0 ];
	// 4'h1: portbemux = pages[15:8 ];
	// 4'h2: portbemux = pages[23:16];
	// 4'h3: portbemux = pages[31:24];
	// 4'h4: portbemux = pages[39:32];
	// 4'h5: portbemux = pages[47:40];
	// 4'h6: portbemux = pages[55:48];
	// 4'h7: portbemux = pages[63:56];

	4'h8: portbemux = ramnroms;
	4'h9: portbemux = dos7ffds;

	4'hA: portbemux = p7ffd;
	4'hB: portbemux = peff7_int;

	// 4'hC: portbemux = { ~atm_pen2, atm_cpm_n, ~atm_pen, 1'bX, atm_turbo, atm_scr_mode };


	default: portbemux = 8'bXXXXXXXX;

	endcase

`endif




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

