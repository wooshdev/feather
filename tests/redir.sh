#!/usr/bin/bash
for n in {1..100}; do
	wget --quiet --max-redirect 0 http://localhost/
done
echo "done"
