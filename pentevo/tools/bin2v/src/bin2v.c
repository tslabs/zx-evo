#include <stdio.h>
#include <stdlib.h>


int emit_header(FILE * infile, int abits);
int emit_body(FILE * infile, FILE * outfile, int inlen, int abits);
int emit_footer(FILE * infile);


int main( int argc, char* argv[] )
{
	int error=0;

	FILE * infile=NULL;
	FILE * outfile=NULL;

	int inlen;
	int abits;
	int temp;

	if( argc!=3 )
	{
		printf("usage: bin2v <infile> <outfile>\n\n");
		error++;
	}

	if( !error )
	{
		if( !(infile=fopen(argv[1],"rb")) )
		{
			printf("cant open infile!\n");
			error++;
		}
	}

	if( !error )
	{
		if( !(outfile=fopen(argv[2],"wt")) )
		{
			printf("cant open outfile!\n");
			error++;
		}
	}

	if( !error )
	{
		if( fseek(infile,0,SEEK_END) )
		{
			printf("Cannot fseek() infile!\n");
			error++;
		}
	}

	if( !error )
	{
		inlen=(int)ftell(infile);

		if( inlen==(-1L) )
		{
			printf("Cannot ftell() length of infile!\n");
			inlen=0;
			error++;
		}
	}

	if( !error )
	{
		if( !inlen )
		{
			printf("Infile is zero length!\n");
			error++;
		}
	}

	if( !error && fseek(infile,0,SEEK_SET) )
	{
		printf("Cannot fseek() infile!\n");
		error++;
	}


	if( !error )
	{
		// how many address bits to use
		temp=inlen-1;
		abits=1;
		while( temp>>=1 ) abits++;
	}


	if( !error ) error+=emit_header(outfile, abits);

	if( !error ) error+=emit_body(infile, outfile, inlen, abits);

	if( !error ) error+=emit_footer(outfile);





	if( outfile ) fclose(outfile);
	if( infile  ) fclose(infile);

	return error;
}

int emit_body(FILE * infile, FILE * outfile, int len, int bits)
{
	int i;
	unsigned char b;

	for(i=0;i<len;i++)
	{
		if( 1==fread(&b,1,1,infile) )
		{
			if( b!=0xFF )
				fprintf(outfile, "\t\t%d'h%X: out_word = 8'h%02X;\n", bits,i,(int)b);
		}
		else // error reading byte
		{
			return 1;
		}
	}

	return 0;
}


int emit_header(FILE * file, int abits)
{
	fprintf(file,"// bin2v output\n");
	fprintf(file,"// \n\n");

	fprintf(file,"module bin2v(\n\n");

	fprintf(file,"\tinput  wire [%2d:0] in_addr,\n\n",abits-1);

	fprintf(file,"\toutput reg  [ 7:0] out_word\n\n");

	fprintf(file,");\n\n");

	fprintf(file,"\talways @*\n");

	fprintf(file,"\tcase( in_addr )\n\n");

	return 0;
}

int emit_footer(FILE * file)
{
	fprintf(file,"\n\t\tdefault: out_word = 8'hFF;\n\n");

	fprintf(file,"\tendcase\n\n");

	fprintf(file,"endmodule\n");

	return 0;
}


