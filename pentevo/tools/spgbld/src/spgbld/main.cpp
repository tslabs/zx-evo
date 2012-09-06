
#include "stdafx.h"
#include "defs.h"
#include "msg.h"
#include "func.h"

HDR hdr;
BLK blk[256];
CONF conf;
	
int _tmain(int argc, _TCHAR* argv[])
{
	init_hdr();
	
	if (!wcscmp(argv[1], L"-b"))
		//Build
	{
		conf.packer = -1;
		if (argc > 4 && !wcscmp(argv[4], L"-c"))
			conf.packer = _wtof(argv[5]);
		load_ini(argv[2]);
		load_files();
		if (conf.packer != 0)
			pack_blocks();
		save_out(argv[3]);
		error(RC_OK);
	}
	
	if (!wcscmp(argv[1], L"-i"))
		//Info
	{
		return 0;
	}
	
	if (!wcscmp(argv[1], L"-u"))
		//Unpack
	{
		return 0;
	}
	
	if (!wcscmp(argv[1], L"-r"))
		//Re-pack
	{
		return 0;
	}
	
	print_help();
	return RC_NOARG;
}

