# Bink Compat

Wrapper for RAD Bink runtime DLLs.

On 32-bit Windows this loads `binkw32.dll`, matching the original Arcanum
runtime. On 64-bit Windows it can load a compatible 64-bit Bink runtime
(`binkw64.dll` or `bink2w64.dll`) when one is supplied next to the executable.

The header stuff is courtesy of [Jedi Outcast](https://github.com/ioquake/jedi-outcast) (specifically [bink.h](https://github.com/ioquake/jedi-outcast/blob/master/code/win32/bink.h)).
