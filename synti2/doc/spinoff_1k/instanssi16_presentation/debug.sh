if [ $# -lt 1 ]
then
   echo What to debug?
fi

sad=`objdump -f "$1" | grep "start address" | cut -d' ' -f3`

echo "break *$sad" > tmp.gdfile
echo 'display /3i $rip' >> tmp.gdfile
echo "run" >> tmp.gdfile
echo 'disassemble $rip,+50' >> tmp.gdfile
gdb -x tmp.gdfile $1
