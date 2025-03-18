#!/bin/sh

docker run -it --rm --cap-add=NET_ADMIN --device=/dev/net/tun -v .:/src -p 2222:22 -p 8080:80 exmips_env
