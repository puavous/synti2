#!/bin/sed -f

# This is for trying out shaders without an auxiliary minifier
# tool. (For example, shader_minifier.exe requires mono which might
# not be found on all systems... Try to remove whitespace and
# double-slash comments.  Regarding other things no can do...

s/^ *//g
s/  */ /g
s/ *\([=+\*,<>;//]\) */\1/g
s/\(.*\)\/\/.*$/\1/g
s/\(.*\)/\1/g
