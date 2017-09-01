#!/bin/bash

# THIS IS CURRENTLY INCOMPATIBLE WITH THE PARSER,
# WHICH REQUIRES EACH FRAME TO BE ON A NEW LINE.
# UNFORTUNATELY THERE IS NO CONCEIVABLE WAY FOR ME TO PROCEDURALLY IDENTIFY THE SYNC BYTES,
# SO IT LOOKS LIKE WE'LL HAVE TO CHANGE THE PARSER.

# THIS SCRIPT WILL PROCESS ALL .PCAP FILES IN THIS DIRECTORY,
# AND THEN RUN THE PARSER ON THEM.

# THE PROCESSING REMOVES ALL TCP PACKETS FROM THE PCAP,
# REMOVES EVERYTHING PRECEEDING THE SYNC BYTE IN EACH FRAME, (this is the part that takes so much time)
# THEN DOES A COUPLE MORE THINGS TO MAKE IT READABLE BY THE PARSER,
# LIKE REMOVE NEWLINES AND SPACES.


PCAP=$(find ./thePcaps -name "*.pcap" )


for pcap in $PCAP
do
  
  echo -e "\nParsing..."
  echo $pcap | cut -d "/" -f 3

  tshark -x -r $pcap | cut -c 7-55 > allhex.txt
  # Extracts the hex with the newlines between the frames

  breaks=$(grep -nvE '[^[:space:]]' allhex.txt | sed s/://g)
  # Gets a list of the line-numbers of the newline separations

  # This will remove the TCP packets
  # Iterations
  # 1 - Compare break to start
  # If the difference is exactly 5, replace the frame with empty lines
  # 2 - Update start

  start=1

  offset=0
  # This variable will account for the change in the number of lines when deleting

  for break in $breaks
  do
    if [ $start -eq $(($break-$offset-5)) ]
    then
      	sed "$start",$(($break-$offset))d allhex.txt > tmp.txt
		cat tmp.txt > allhex.txt
		# Replaces the garbage lines with empty lines

		(( offset += 6 ))

    fi

  start=$(($break + 1 - $offset))
  # Moves the "start" pointer to the beginning of the next frame

  done

  # Here is the best place to trim the garbage
  sed 's/ aa/\
+aa/' allhex.txt > tmp.txt
  	cat tmp.txt > allhex.txt


  SYNCLines=$(grep -n "aa" allhex.txt | cut -d : -f 1)
  # Gets the lines with the sync bytes in them


  backup=1
  # This will be used to delete the lines behind the SYNC line

 
  # For each SYNC line,
  # Insert a newline before the SYNC byte
  # Delete previous lines until an empty line

  for Line in $SYNCLines
  do

  	# Until the deletion has backed up to either a line break or the end of the file...
  	until [[ -z $(sed -n $(($Line-$backup))p allhex.txt) || $(($Line-$backup)) == 0 ]]
  	do
  		sed $(($Line-$backup))g allhex.txt > tmp.txt
  		cat tmp.txt > allhex.txt

  		((backup++))
  	done

  	let backup=1
  	# Resets for the next SYNC line
  done

  sed -e ':a' -e 'N' -e '$!ba' -e 's/\n/ /g' allhex.txt | sed s/" "//g > fulltest.txt
  # Removes all of the new lines and spaces
  sed 's/+//' fulltest.txt > tmp.txt
  # Deletes the first +
  tr + '\n' <tmp.txt > fulltest.txt
  # Replaces all subsequent ones with newlines
  echo '' >> fulltest.txt
  # Adds a final newline


  rm allhex.txt
  rm tmp.txt

  ./dt
  #Runs the parser!

done

