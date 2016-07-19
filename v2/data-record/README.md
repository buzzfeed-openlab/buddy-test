Tests will have 3 associated files:
*_raw
*_time
*_peaks
*_data.csv

You can read the *_processed file using index.html

Process the files with:

awk 'BEGIN{FS=",";OFS=","};{print $2,$3}' [NAME]_raw > [NAME]_peaks

awk 'BEGIN{FS=",";OFS=","};{print $1}' [NAME]_raw | awk 'BEGIN{FS=":";OFS=":"};{print $1*3600+$2*60+$3-([hours to subtract]*3600+[minutes to subtract]*60+[seconds to subtract])}' | awk 'BEGIN{OFS=":"}; {print int($1/3600),int(($1%3600)/60),int(($1%3600)%60)}' > [NAME]_time

cut -d, -f2 [NAME]_peaks | paste -d, [NAME]_time - > [NAME]_data.csv


Or, to preserve normal time:
awk 'BEGIN{FS=",";OFS=","};{print $2,$3}' [NAME]_raw > [NAME]_data.csv

Then use:
awk -F"," '!_[$1]++' [NAME]_data.csv >[NAME]_data_uniq.csv

then you should:

cat headers [NAME]_data[_uniq].csv >data.csv

to display it

If you want to then convert the time to the timestamps of the movie (zero the beginning stamp) I suggest:

awk 'BEGIN{FS=",";OFS=","};{print $1}' [NAME]_data_uniq.csv | awk 'BEGIN{FS=":";OFS=":"};{print $1*3600+$2*60+$3-([hours to subtract]*3600+[minutes to subtract]*60+[seconds to subtract])}' | awk 'BEGIN{OFS=":"}; {print int($1/3600),int(($1%3600)/60),int(($1%3600)%60)}' > [NAME]_time

awk 'BEGIN{FS=","};{print$2}' [NAME]_data_uniq.csv >[NAME]_peaks

paste -d, [NAME]_time [NAME]_peaks >[NAME]_data_zeroed.csv