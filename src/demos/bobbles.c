
const char data[] = {0xff,0xaa};
void out_xt (unsigned char, unsigned char);

void main()
{
	unsigned int i = 0;

	while(i<=7)
	{
//		if( data[i] == 0 ) break;
		out_xt(i, data[i]);
		i++;

	}
}

	
 void out_xt (unsigned char r, unsigned char v)

 {	__asm
	ld bc,#0x55ff
	ld e,#0xaa
	out (c),e
	ret
	__endasm;
}
	



