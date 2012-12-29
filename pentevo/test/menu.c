
void msg_disp()
{
	// switch(menu)
	// {
		// case M_MAIN:
			// { msg_main(); break; }
	// }
}

void key_disp()
{
	u8 key;

	while(1)
	{
		key = getkey();
		switch(menu)
		{
			case M_MAIN:
			{
				switch(key)
				{
					case K_0: { menu = M_SAVE; return; }
				}
				break;
			}
		}
	}
}
