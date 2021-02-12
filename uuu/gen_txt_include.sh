#!/bin/sh

echo "R\"####(" > $2
cat $1 >> $2
echo ")####\"" >> $2
