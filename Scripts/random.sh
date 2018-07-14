#!/bin/bash    
    
function rand(){    
    min=$1
    max=$(($2-$min+1))    
    num=$(date +%s%N)    
    echo $(($num%$max+$min))    
}    
    
for ((i=0; i<500000; i++)); do
	year=$(rand 1900  2020)    
	month=$(rand 1 12)
	day=$(rand 1 30)
	tim=$(rand 0 23)
	min=$(rand 0 59)
	sec=$(rand 0 59)
	touch ${year}_${month}_${day}_${tim}_${min}_${sec}.jpeg
done
