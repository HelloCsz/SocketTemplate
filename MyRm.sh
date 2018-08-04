#!/bin/bash
#filename:MyRM.sh
#author:camel_flying
#add a line "alias rm='MyRM.sh'" to ~/.bashrc
#then copy this file to /usr/bin                                                                                 
  
MAX=1024*1024      #1G
 
if [ ! -d  ~/.local/share/Trash/files/ ] ;then
        mkdir ~/.local/share/Trash/files/
fi
     
line=`du -cs $@ |tail -n 1`
size=`echo $line |cut -d' ' -f1`
      
if (( $size < $MAX )); then
        echo "mv $@ ~/.local/share/Trash/files/"
        mv $@ ~/.local/share/Trash/files/
else
        echo "/usr/bin/rm -i $@"
        /bin/rm -i $@
fi
