# generate 2014 app sizes
if [ ! -f 2014.csv ]; then
	cat ../app-list/apps_2014.csv | rev |  cut -f2 -d';' | rev > 2014.csv
fi

# generate 2019 app sizes
if [ ! -f 2019.csv ]; then
	cat ../app-list/apps_2019.csv | rev | cut -f3 -d';' | rev > 2019.csv
fi

# Average app size
cat 2014.csv | awk '{ sum += $1 } END { if (NR > 0) print sum / (1024*1024*NR) }'
cat 2019.csv | awk '{ sum += $1 } END { if (NR > 0) print sum / (1024*1024*NR) }'
