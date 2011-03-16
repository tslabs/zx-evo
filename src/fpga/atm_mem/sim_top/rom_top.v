// full zxevo ROM here (pentagon paging: gluk(0)/trdos(1)/bas128(2)/bas48(3)

module rom(
	input [15:0] addr,
	output reg [7:0] data,
	input ce_n
);


	wire [7:0] word;

	bin2v zxevo_rom( .in_addr(addr), .out_word(word) );


	always @*
	begin
		if( !ce_n )
			data = word;
		else
			data = 8'bZZZZZZZZ;

	end




endmodule

