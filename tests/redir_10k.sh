#!/usr/bin/bash 
for n in {1..10000}; do 
	wget --quiet -max-redirect 0 http://localhost/
done
