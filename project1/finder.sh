#!/bin/bash

x=$(find outfolder -name *.out)         #get all files that end in .out
arg=$*                                  #program args
for tld in $arg
do
    counter=0
    for i in $x                                 #for each file
    do 
        exec < $i                       
        while read line                         #read line by line the file
        do
            if [[ $line == *"."$tld* ]]                 #.tld is substring of line        
            then
                let counter=${line: -1}+counter         #line: -1 get the last character of line 
                                                        #which is the number of appearances
            fi
        done
    done
    echo ".$tld appeared in $counter location urls"
done
