#!/usr/bin/perl -pi~

s{ (\w+) (\s*=) }{ ($c{$1}//=$i++).$2 }gex; 
s{ ([a-z]\w*) (\s*[,)]) }{ ($c{$1}//=$i++).$2 }gex; 
END { print 0+(sort values %c)[-1] }
