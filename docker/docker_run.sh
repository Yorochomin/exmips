#!/bin/sh

docker run -it --rm --cap-add=NET_ADMIN --device=/dev/net/tun -v .:/src exmips_env
