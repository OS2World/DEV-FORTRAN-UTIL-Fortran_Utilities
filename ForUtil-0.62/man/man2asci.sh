#!/bin/sh
# convert preformatted manual pages to ascii
#

for file in $* ; do
	echo "Converting $file"
	groff -Tascii -man $file | sed -e 's/.//g' > `basename $file`.txt
done
