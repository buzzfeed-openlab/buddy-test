require 'csv'

filelist=[]

ARGV.each do|a|
	filelist.push(a)
end

def join_next(initial_hsh, next_hsh)
  next_hsh.inject(initial_hsh) do |results, next_hsh|
    key = next_hsh[:date]
    results[key] ||= {}
    results[key] = results[key].merge(next_hsh)
    results
  end
end


def combine_data(array_of_hashes)
	hsh={}
	array_of_hashes.each do|c|
		hsh = join_next(hsh,c)
	end
	return hsh.values
end

def export_as_csv(filename, array_of_hashes, names)
	array_of_hashes.each do|d|
		names.each do |e|
			if not d.key?(e) then
				d[e] = nil
			end
		end
	end
	s=CSV.generate do |csv|
		csv << names
		array_of_hashes.each do |x|
			line=[]
			names.each do |y|
				line.push(x[y])
			end
			csv << line
		end
	end
	File.write(filename, s)
end

#filelist=['01_final.csv','02_final.csv','03_final.csv']

hasharray=[]


csv_parse_options = {
  headers: true,
  header_converters: :symbol
}

filelist.each do|b|
#	hasharray.push(load(b))
	opened_csv=CSV.read(b, csv_parse_options)
	hasharray.push(opened_csv.map(&:to_hash))
end

scaled_hasharray=hasharray.clone

# scale by percent here
scaled_hasharray.each do|i|
	myKey=i.first.keys[1]
	# get min and max
	maxNum=0
	i.each do |r|
		if Integer(r[myKey])>maxNum then
			maxNum=Integer(r[myKey])
		end
	end
	minNum=maxNum
	i.each do |r|
		if Integer(r[myKey])<minNum then
			minNum=Integer(r[myKey])
		end
	end
	i.each do |r|
		newValue=(Integer(r[myKey])-minNum)*100/(maxNum-minNum)
		if ((Integer(r[myKey])-minNum)*100) % (maxNum-minNum) > (maxNum-minNum)/2 then
			newValue=newValue+1
		end
		r[myKey]=String(newValue)
	end
end

final=combine_data(hasharray)
column_names = final.first.keys

scaled_final=combine_data(scaled_hasharray)

# go through final
# if a key from column_names is not present in any of the rows, then add a key with an empty string
# then you can export

export_as_csv('data.csv',final,column_names)
export_as_csv('scaled_data.csv',scaled_final,column_names)



