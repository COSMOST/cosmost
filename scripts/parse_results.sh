#find . -name 
cat $1 | while read line
	do
		app=$line
		#mem=$(cat $line | grep ",\"Memory Usage\",Memory Usage \[KB]," | rev | cut -f1 -d, | rev | head -n1)
		#cpu=$(cat $line | grep "\"CPU Load\",CPU Load \[%]," | rev | cut -f1 -d, | rev | head -n1)
		bat=$(cat $line/$2.csv | grep "\"Battery Power\*\",Power \[uW]," | rev | cut -f1 -d, | rev | head -n1)
		#echo $app,$mem,$cpu,$bat
		echo $app,$bat,$bat
	done
