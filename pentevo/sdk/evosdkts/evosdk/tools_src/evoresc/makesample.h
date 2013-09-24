unsigned char* load_wav(char *name,int& size,int& rate)
{
	FILE *file;
	unsigned char *wave,*snd;
	int aa,bb,samples,channels;
	int bits,ptr,pd,smp;
	bool fl;

	file=fopen(name,"rb");

	if(!file)
	{
		printf("ERR: Can't open sample (%s)\n",name);
		return NULL;
	}
	
	fseek(file,0,SEEK_END);
	size=ftell(file);
	fseek(file,0,SEEK_SET);
	
	wave=(unsigned char*)malloc(size);

	if(!wave)
	{
		printf("ERR: Can`t allocate memory.\n");
		return NULL;
	}

	fread(wave,size,1,file);
	fclose(file);
	
	fl=false;

	for(aa=0;aa<size-4;aa++)
	{
		if(memcmp(&wave[aa],"RIFF",4)==0)
		{
			fl=true;
			ptr=aa;
			break;
		}
	}

	if(!fl)
	{
		printf("ERR: RIFF chunk not found\n");
		free(wave);
		return NULL;
	}

	fl=false;

	for(aa=ptr;aa<size-4;aa++)
	{
		if(!memcmp(&wave[aa],"WAVEfmt ",8))
		{
			fl=true;
			ptr=aa;
			break;
		}
	}

	if(!fl)
	{
		printf("ERR: WAVEfmt chunk not found\n");
		free(wave);
		return NULL;
	}

	if(read_word(&wave[ptr+12])!=1)
	{
		printf("ERR: Only unpacked PCM supported (%s)\n",name);
		free(wave);
		return NULL;
	}

	channels=read_word(&wave[ptr+14]);
	rate=read_dword(&wave[ptr+16]);
	bits=read_word(&wave[ptr+26]);

	if(channels<1||channels>2)
	{
		printf("ERR: Only mono/stereo files supported (%s)\n",name);
		free(wave);
		return NULL;
	}

	if(bits!=8&&bits!=16)
	{
		printf("ERR: Only 8/16bit PCM supported (%s)\n",name);
		free(wave);
		return NULL;
	}

	switch(rate)
	{
	case 8000:  rate=27; break;//7918
	case 11025: rate=18; break;//11075
	case 16000: rate=11; break;//16055
	case 22000:
	case 22050: rate=7;  break;//21604
	case 32000: rate=3;  break;//33018
	case 44100: rate=1;  break;//43750
	default:
		printf("ERR: Only 44100 Hz or less supported (%s)\n",name);
		free(wave);
		return NULL;
	}

	fl=false;

	for(aa=ptr+28;aa<size-4;aa++)
	{
		if(!memcmp(&wave[aa],"data",4))
		{
			fl=true;
			ptr=aa;
			break;
		}
	}

	if(!fl)
	{
		printf("ERR: DATA chunk not found\n");
		free(wave);
		return NULL;
	}
	
	samples=read_dword(&wave[ptr+4])/channels/(bits>>3);
	
	ptr+=8;
	
	snd=(unsigned char*)malloc(samples+1);
	
	pd=0;

	for(aa=0;aa<samples;aa++)
	{	
		smp=0;

		for(bb=0;bb<channels;bb++)
		{
			switch(bits)
			{
			case 8:
				smp+=(signed char)(wave[ptr]+128);
				ptr++;
				break;

			case 16:
				smp+=((signed char)wave[ptr+1]);
				ptr+=2;
				break;
			}
		}

		smp=0x80+smp/channels;

		if(!smp) ++smp;//в сэмпле не должно быть нулей

		snd[pd++]=smp;
	}
	
	snd[pd++]=0;//конец сэмпла

	size=pd;

	free(wave);

	return snd;
}