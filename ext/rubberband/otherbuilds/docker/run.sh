#!/bin/sh

revision=$(hg id | sed 's/[^0-9a-z].*$//')

cat Dockerfile.in | perl -p -e "s/\[\[REVISION\]\]/$revision/g" > Dockerfile

sudo docker build -f Dockerfile .

