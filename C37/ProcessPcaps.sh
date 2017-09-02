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

  editcap -C 66 $pcap "headlessPCAP.pcap"

  tshark -x -r headlessPCAP.pcap | cut -c 7-55 > allhex.txt
  # Extracts the hex with the newlines between the frames


  sed 's/^aa/\
+aa/' allhex.txt > tmp.txt
  cat tmp.txt > allhex.txt
  # Puts a '+' in front of each SYNC byte to mark them

  sed 1,$(($(grep -n -m 1 "\+" allhex.txt | cut -d : -f 1) - 1))d allhex.txt > tmp.txt
  cat tmp.txt > allhex.txt
  # Removes the first big couple of header packets

  SYNCLines=$(grep -n "\+aa" allhex.txt | cut -d : -f 1)
  # Gets the lines with the sync bytes in them

  sed -e ':a' -e 'N' -e '$!ba' -e 's/\n/ /g' allhex.txt | sed s/" "//g  | sed 1's/\+//' | sed 's/\+/\
/g' > fulltest.txt
  # Removes the new lines and spaces
  # Removes the first '+'
  # Replaces the rest of the '+'s with newlines

  rm headlessPCAP.pcap
  rm allhex.txt
  rm tmp.txt

  ./dt
  #Runs the parser!

done
