//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::               by dr_max^gc (c)2018-2019                 ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcMouseInit(u8 gfxpage) __naked;
void gcMouseUpdate(void) __naked;
u16 gcGetMouseX(void);
u16 gcGetMouseY(void);
u8 gcGetMouseXS(void);
u8 gcGetMouseYS(void);
u8 gcGetMouseWheel(void);
