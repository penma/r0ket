#!/bin/bash
#set -x
verbmsg()
{
true
# echo $1
}

if test ! -d simulat0r/firmware -o ! -d firmware
then
echo ERROR:
echo This script must be run from toplevel r0ket directory
exit
fi

echo "Updating directories"
for i in `find firmware -type d `
do 
if test -d simulat0r/$i
then verbmsg "OK Directory already exists: $i"
else mkdir -v simulat0r/$i
fi
done

echo "Updating bridge files for C source"
for i in `find firmware \! -path firmware/lcd/allfonts.h \! -path firmware/l0dable/usetable.h -type f -iname \*.[ch]`
do 
    if test -f simulat0r/$i; 
    then
	verbmsg "OK File already exists: $i" 
    else
	echo Writing bridge file simulat0r/$i
	(printf "/* AUTOGENERATED SOURCE FILE */\n"; echo \#include \"`dirname $i | sed "s#[^/]*#..#g" `/../$i\") >simulat0r/$i
    fi
done

echo "Looking for dangling bridge files (i.e. the original file no longer exists)"
for i in `find simulat0r/firmware \! -path simulat0r/firmware/libc-unc0llide.h -type f -iname \*.[ch] | sed "s#simulat0r/##"`
do
    if test -f $i;
    then
        verbmsg "OK File still exists: $i"
    else
#	echo Introspecting dangling bridge file simulat0r/$i
        if cmp -s simulat0r/$i <(printf "/* AUTOGENERATED SOURCE FILE */\n"; echo \#include \"`dirname $i | sed "s#[^/]*#..#g" `/../$i\") ;
	then
            echo Dangling unchanged bridge file simulat0r/$i
	    SUGGESTDEL="$SUGGESTDEL simulat0r/$i"
	else
            echo Dangling modified bridge file simulat0r/$i
	fi
    fi
done
if test "$SUGGESTDEL"; 
then
    echo You might want to delete unneeded unmodified bridge files:
    echo
    echo rm $SUGGESTDEL
    echo git rm $SUGGESTDEL
    echo
fi


echo "Updating bridge files for Makefiles"
for i in `find firmware -type f -iname Makefile`
do 
    if test -f simulat0r/$i; 
    then
	verbmsg "OK File already exists: $i" 
    else
	echo Writing bridge file simulat0r/$i
	(printf "# GENERATED INCLUDE BRIDGE/\n"; echo include `dirname $i | sed "s#[^/]*#..#g" `/../$i) >simulat0r/$i
    fi
done


for i in `find firmware -type f -name .gitignore`; do
    if cmp $i simulat0r/$i
    then
	echo OK $i and simulat0r/$i are the same
    else
	echo WARNING: $i mismatches simulat0r/$i
	echo "   give this command a try if the following diff looks reasonable:"
	echo "   cp $i simulat0r/$i"
	diff -y $i simulat0r/$i || true
    fi 
done
