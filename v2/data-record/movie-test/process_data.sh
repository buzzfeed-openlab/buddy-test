#!/bin/bash

# a script to process raw data from buddy

suffix_raw="_raw.txt"
suffix_data="_data.csv"
suffix_uniq="_data_uniq.csv"
suffix_zeroed="_data_zeroed.csv"
suffix_time="_time"
suffix_peaks="_peaks"
suffix_final="_final.csv"

echo -n "Enter file prefix: "
read prefix
echo "entered $prefix"

echo "Now processing $prefix$suffix_raw..."

awk 'BEGIN{FS=",";OFS=","};{print $2,$3}' $prefix$suffix_raw > $prefix$suffix_data

echo "data sample: "
head -1 $prefix$suffix_data

awk -F"," '!_[$1]++' $prefix$suffix_data >$prefix$suffix_uniq

echo "unique data sample: "
head -1 $prefix$suffix_uniq

#file = "$prefix$suffix_uniq"
#while IFS=':' read -r f1 f2 f3
#do 
#	printf 'hour: %s, minute: $s, second' "$f1" "$f2" "$f3"
#done < "$file"

data_hour=`awk 'BEGIN{FS=",";OFS=","};{print $1}' $prefix$suffix_uniq | awk 'BEGIN{FS=":"};{print $1}' | head -1`
data_minute=`awk 'BEGIN{FS=",";OFS=","};{print $1}' $prefix$suffix_uniq | awk 'BEGIN{FS=":"};{print $2}' | head -1`
data_second=`awk 'BEGIN{FS=",";OFS=","};{print $1}' $prefix$suffix_uniq | awk 'BEGIN{FS=":"};{print $3}' | head -1`

echo "Start time was $data_hour:$data_minute:$data_second."
time_factor=`expr $data_hour \\* 3600 + $data_minute \\* 60 +  $data_second`
echo "Time factor is $time_factor."

awk 'BEGIN{FS=",";OFS=","};{print $1}' $prefix$suffix_uniq | awk -v time_factor="$time_factor" 'BEGIN{FS=":";OFS=":"};{print $1*3600+$2*60+$3-time_factor}' | awk 'BEGIN{OFS=":"}; {print int($1/3600),int(($1%3600)/60),int(($1%3600)%60)}' >$prefix$suffix_time

awk 'BEGIN{FS=","};{print$2}' $prefix$suffix_uniq >$prefix$suffix_peaks

paste -d, $prefix$suffix_time $prefix$suffix_peaks >$prefix$suffix_zeroed

hour_sum=`awk 'BEGIN{FS=":"};{sum+=$1} END {print sum}' $prefix$suffix_zeroed`

if [ $hour_sum -eq 0 ];
then
	# layout for minutes
	{ echo -n 'date,'$prefix'
'; awk 'BEGIN{FS=":";OFS=":"};{print $2,$3}' $prefix$suffix_zeroed; } > $prefix$suffix_final
else
	# layout for hours
{ echo -n 'date,'$prefix'
'; cat $prefix$suffix_zeroed; } > $prefix$suffix_final
fi

