# LaTeX2HTML 2002-2-1 (1.71)
# Associate images original text with physical files.


$key = q/^{textrm{2}};MSF=1.6;AAT/;
$cached_env_img{$key} = q|<IMG
 WIDTH="10" HEIGHT="20" ALIGN="BOTTOM" BORDER="0"
 SRC="|."$dir".q|img1.png"
 ALT="$ ^{\textrm{2}}$">|; 

$key = q/^{text{2}};MSF=1.6;AAT/;
$cached_env_img{$key} = q|<IMG
 WIDTH="10" HEIGHT="20" ALIGN="BOTTOM" BORDER="0"
 SRC="|."$dir".q|img3.png"
 ALT="$ ^{\text{2}}$">|; 

$key = q/^{text{TM}};MSF=1.6;AAT/;
$cached_env_img{$key} = q|<IMG
 WIDTH="22" HEIGHT="20" ALIGN="BOTTOM" BORDER="0"
 SRC="|."$dir".q|img2.png"
 ALT="$ ^{\text{TM}}$">|; 

$key = q/Omega;MSF=1.6;AAT/;
$cached_env_img{$key} = q|<IMG
 WIDTH="16" HEIGHT="14" ALIGN="BOTTOM" BORDER="0"
 SRC="|."$dir".q|img4.png"
 ALT="$ \Omega$">|; 

1;

