module slavespi(

	input fclk,
	input rst_n,

	input spics_n,
	output reg spidi,
	input spido,
	input spick,


	input [15:0] a,
	output reg [4:0] keyout,


	output genrst,
	output reg [1:0] rstrom
);


	reg [7:0] reset_reg;

	always @(posedge spick, negedge spics_n)
	begin
		if( !spics_n )
			reset_reg[7:0] <= 8'd0;
		else // posedge spick
			reset_reg[7:0] <= { spido, reset_reg[7:1] };
	end

	always @(negedge spick)
	begin
		if( genrst ) rstrom <= reset_reg[5:4];
	end

	assign genrst = reset_reg[1];



	reg [39:0] kym;  //
	wire [4:0] keys [0:7]; // key matrix
	reg [4:0] keyreg [0:7];

	reg ksync1,ksync2,ksync3;


	always @(posedge spick) if( ~spics_n )
	begin
		kym[39:0] <= { spido, kym[39:1] };
	end

	assign keys[0][4:0] = { kym[00],kym[08],kym[16],kym[24],kym[32] };
	assign keys[1][4:0] = { kym[01],kym[09],kym[17],kym[25],kym[33] };
	assign keys[2][4:0] = { kym[02],kym[10],kym[18],kym[26],kym[34] };
	assign keys[3][4:0] = { kym[03],kym[11],kym[19],kym[27],kym[35] };
	assign keys[4][4:0] = { kym[04],kym[12],kym[20],kym[28],kym[36] };
	assign keys[5][4:0] = { kym[05],kym[13],kym[21],kym[29],kym[37] };
	assign keys[6][4:0] = { kym[06],kym[14],kym[22],kym[30],kym[38] };
	assign keys[7][4:0] = { kym[07],kym[15],kym[23],kym[31],kym[39] };

	always @(posedge fclk)
	begin
		ksync1 <= spics_n;
		ksync2 <= ksync1;
		ksync3 <= ksync2;

		if( ksync2 && (~ksync3) )
		begin
			keyreg[0] <= keys[0];
			keyreg[1] <= keys[1];
			keyreg[2] <= keys[2];
			keyreg[3] <= keys[3];
			keyreg[4] <= keys[4];
			keyreg[5] <= keys[5];
			keyreg[6] <= keys[6];
			keyreg[7] <= keys[7];
		end
	end



	always @*
	begin
		keyout = 5'b11111;

		if( ~a[8] )
			keyout = keyout & (~keyreg[0]);

		if( ~a[9] )
			keyout = keyout & (~keyreg[1]);

		if( ~a[10] )
			keyout = keyout & (~keyreg[2]);

		if( ~a[11] )
			keyout = keyout & (~keyreg[3]);

		if( ~a[12] )
			keyout = keyout & (~keyreg[4]);

		if( ~a[13] )
			keyout = keyout & (~keyreg[5]);

		if( ~a[14] )
			keyout = keyout & (~keyreg[6]);

		if( ~a[15] )
			keyout = keyout & (~keyreg[7]);
	end




endmodule

