# Builder-TinyCC
Embarcadero Rad Studio C++ Builder TinyCC

tcc-0.9.27.tar.bz2 orginal tcc from http://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27.tar.bz2

to patch :<br>
&nbsp;  extract tcc-0.9.27.tar.bz2 into folder tcc<br>
&nbsp;  cd tcc<br>
&nbsp;  patch -p0 < ../tccbrlnd.patch<br>
  
after build:<br>
&nbsp; copy folder tcc\include into build folder i.e: Win64\Debug<br>
&nbsp; copy folder tcc\win32\include into build folder i.e: Win64\Debug<br>
&nbsp; copy folder tcc\win32\lib into build folder i.e: Win64\Debug<br>

run hello.c:<br>
&nbsp; cd Win64\Debug<br>
&nbsp; tcc -run -D_WIN -luser32 hello.c
