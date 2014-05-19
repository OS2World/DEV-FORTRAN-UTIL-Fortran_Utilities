#!/bin/sh
#
# add_urls.sh: add cross references to the html pages outputted by man2html
#
# set this to 1 if you want output
verbose=0
if [ "$verbose" = "1" ]; then
	echo -n "Adding HTML crossreferences: "
fi
for file in html/*.html ; do
	curr_file=`basename $file .html`
	if [ "$verbose" = "1" ]; then
		echo -n "$curr_file.html "
	fi
	tmp_cache=$curr_file.cache
	for html_page in html/*.html ; do
		this_file=`basename $html_page .html`
		if [ ! "$curr_file" = "$this_file" ] ; then
cat >> $tmp_cache << !EO_CACHE
s@<STRONG>$this_file</STRONG>@<a href="$this_file.html">$this_file</a>@
!EO_CACHE
		fi
	done
	cp $file temp.html
	sed -f $tmp_cache temp.html > $file
	rm temp.html
	rm $tmp_cache
done
if [ "$verbose" = "1" ]; then
	echo "done"
fi
