To compile type: make release


compile options:

DEBUG=1: Compile in debug mode with debug output to "SEPLUGINS/joysens.log"

LIBM=1: Compile with full libmath support (more bloat)

NOCONFIG=1: Compile without config file support (no config file settings will be read or written)

NOCONFIGSAVE=1: Only without NOCONFIG. Disable config file saving support.

LITE=1: Compile the lite version, which does not support config file saving (but reading by default),
        no info output support as well as no ingame config setting through button combinations.
        Alternatively, you can just type "make lite"
