
svn co "http://u8u16.costar.sfu.ca/svn/trunk/"

u8u16/ is the u8u16 trunk (rev 101) as of 24 November 2012. It's unaltered other than
using ctangle (which most systems won't have installed) to create libu8u16.c from libu8u16.w.

The build structure isn't really used, we just build what is needed out of our Makefile. If
you change stuff inside u8u16/ don't expect your changes to be correctly folded into a build.

See u8u16/README and u8u16/LICENSE for more information.

