// This module serves direct DRAM-to-device data transfer


module dma (

// clocks
	input wire clk,
	input wire rst_n,

// controls	
	input wire [7:0] zdata,
	input wire [4:0] dmaport_wr,
	output reg dma_act,
	output reg dma_zwait,

// SD interface
	input wire  [7:0] sd_rddata,
	output wire [7:0] sd_wrdata,

// HDD interface
	
// DRAM interface
	output reg  [20:0] dma_addr,
	input wire  [15:0] dma_rddata,
	output wire [15:0] dma_wrdata,
	output wire dma_req,
	output reg  dma_rnw,
	input wire  dma_next,
	input wire  dma_stb

);


// blocking of DMA regs write strobes while DMA active
	wire [4:0] dma_wr = dmaport_wr & {5{!dma_act}};

	
// target device
	reg [1:0] device;
	wire sd = device == 2'b00;
	wire hdd = device == 2'b10;
	
	
	wire word_ok = 0;
	
	
// 
	always @(posedge clk)
	if (!rst_n)
	begin
		dma_act <= 1'b0;
		dma_zwait <= 1'b0;
	end
	
	else			// write to DMACtrl (launch of DMA burst)
	if (dma_wr[4])
	begin
		device <= zdata[1:0];
		dma_rnw <= zdata[6];
		dma_act <= 1'b1;
	end
	
	else			// word has been processed OK
	if (word_ok)
	begin
		dma_act <= !ctr[8];
	end

	
// DMA processing
	always @(posedge clk)
	if (dma_act)
	begin
		
	end
	
	
// address processing	
	always @(posedge clk)
	begin
		if (dma_next)						// increment on DRAM next addr request
			dma_addr <= dma_addr + 1;
		if (dma_wr[0])						// setting by write to DMAAddrL
			dma_addr[6:0] <= zdata[7:1];
		if (dma_wr[1])						// setting by write to DMAAddrH
			dma_addr[14:7] <= zdata;
		if (dma_wr[2])						// setting by write to DMAAddrX
			dma_addr[20:15] <= zdata[5:0];
	end
	

// counter processing	
	reg [8:0] ctr;
	
	always @(posedge clk)
	begin
		if (word_ok)			// decrement on successfull word processing
			ctr <= ctr - 1;
		if (dma_wr[3])			// setting by write to DMALen
			ctr[7:0] <= zdata;
		if (dma_wr[4])			// launch of DMA burst (write to DMACtrl)
			ctr[8] <= 1'b0;
	end


endmodule
