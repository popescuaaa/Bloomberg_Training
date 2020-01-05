#!/bin/bash
if [ "$#" -ne 3 ]; then
	echo "Illegal numer of arguments"
	echo "The first argument should be the directory where the PNM,PGM directories with input images
		are"
	echo "The second argument should be the name of the executable"
	echo "The third argument should be the directory where the pnm/pgm directories with reference 
		output images are"
	exit
fi
filters=(
	'smooth'
	'blur'
	'sharpen'
	'mean'
	'emboss'
	"blur smooth sharpen emboss mean blur smooth sharpen emboss mean"
	)
for d in $1/*; do
	for file in "$d"/*; do
		type=$(echo $file| cut -d '/' -f2 | tr '[:upper:]' '[:lower:]')
		name=$(basename $file)
		extension=$(echo "$name"| cut -d '.' -f2)
		if [[ "$extension" == "$type" ]]; then
			echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
			echo "$name"
			noext=$(echo "$name" | cut -d "." -f1)
			for filter in "${filters[@]}"; do
				echo "##"
				echo "$filter"
				output="$noext-$filter.$extension"
				if [[ "$filter" == "blur smooth sharpen emboss mean blur smooth sharpen emboss mean" ]]; then
					output="$noext-bssembssem.$extension"
				fi
				eval mpirun -np 4 $2 "$file" "$output" "$filter" &> /dev/null
				diff "$output" "$3"/"$extension"/"$output" && echo "output correct"
				eval mpirun -np 1 $2 "$file" "1proc" "$filter" &> /dev/null
				eval mpirun -np 2 $2 "$file" "2proc" "$filter" &> /dev/null
				eval mpirun -np 3 $2 "$file" "3proc" "$filter" &> /dev/null
				diff "1proc" "2proc" && diff "2proc" "3proc" && diff "3proc" "$output" && echo "the output is the same, no matter the number of processes"
				rm "$output" "1proc" "2proc" "3proc"
			done
			echo ""
		fi
	done
done
