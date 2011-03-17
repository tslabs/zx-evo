module rom(
	input [15:0] addr,
	output reg [7:0] data,
	input ce_n
);

	always @*
	begin
		if( ce_n )
			data <= 8'bZZZZZZZZ;
		else
		case(addr)
			16'h0000: data<=8'h21;
			16'h0001: data<=8'h00;
			16'h0002: data<=8'h00;
			16'h0003: data<=8'h11;
			16'h0004: data<=8'h00;
			16'h0005: data<=8'h80;
			16'h0006: data<=8'h01;
			16'h0007: data<=8'h0e;
			16'h0008: data<=8'h00;
			16'h0009: data<=8'hed;
			16'h000a: data<=8'hb0;
			16'h000b: data<=8'hc3;
			16'h000c: data<=8'h00;
			16'h000d: data<=8'h80;

			default: data<=8'hFF;
		endcase

	end




endmodule

