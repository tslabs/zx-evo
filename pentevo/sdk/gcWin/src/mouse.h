//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::               by dr_max^gc (c)2018-2019                 ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void gcMouseInit(u8 gfxpage) __naked;
void gcMouseUpdate(void) __naked;
inline u16 gcGetMouseX(void);
inline u16 gcGetMouseY(void);
inline u8 gcGetMouseXS(void);
inline u8 gcGetMouseYS(void);
inline u8 gcGetMouseWheel(void);
