#!/usr/bin/bash
for n in {1..100}; do
	bash redir.sh &
done
wait
