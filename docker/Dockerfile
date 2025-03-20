FROM ubuntu:22.04
RUN apt-get update && apt-get install -y iptables net-tools iproute2 build-essential git
RUN echo "if [ ! -f /net_configured ]; then"                  >> /etc/bash.bashrc && \
    echo "   ip tuntap add mode tap name tap0"                >> /etc/bash.bashrc && \
    echo "   ip link set dev tap0 up"                         >> /etc/bash.bashrc && \
    echo "   ip link add dev br0 type bridge"                 >> /etc/bash.bashrc && \
    echo "   ip link set dev br0 up"                          >> /etc/bash.bashrc && \
    echo "   ip link set dev tap0 master br0"                 >> /etc/bash.bashrc && \
    echo "   IP_ADDR=\`hostname -i\`"                         >> /etc/bash.bashrc && \
    echo "   ip address add dev br0 \$IP_ADDR/16"             >> /etc/bash.bashrc && \
    echo "   ip route append default via 172.17.0.1 dev br0"  >> /etc/bash.bashrc && \
    echo "   ip link set dev eth0 master br0"                 >> /etc/bash.bashrc && \
    echo "   ip address del dev eth0 \$IP_ADDR/16"            >> /etc/bash.bashrc && \
    echo "   ip address add dev br0 192.168.100.1/24"         >> /etc/bash.bashrc && \
    echo "   iptables -t nat -A POSTROUTING -o br0 -j MASQUERADE"                                >> /etc/bash.bashrc && \
    echo "   iptables -A FORWARD -i br0 -p tcp -d 192.168.100.10 --dport 22 -j ACCEPT"           >> /etc/bash.bashrc && \
    echo "   iptables -t nat -A PREROUTING -i br0 -p tcp --dport 22 -j DNAT --to 192.168.100.10" >> /etc/bash.bashrc && \
    echo "   iptables -A FORWARD -i br0 -p tcp -d 192.168.100.10 --dport 80 -j ACCEPT"           >> /etc/bash.bashrc && \
    echo "   iptables -t nat -A PREROUTING -i br0 -p tcp --dport 80 -j DNAT --to 192.168.100.10" >> /etc/bash.bashrc && \
    echo "   touch /net_configured"                           >> /etc/bash.bashrc && \
    echo "   echo Network configuration was done"             >> /etc/bash.bashrc && \
    echo "fi"                                                 >> /etc/bash.bashrc && \
    echo ""                                                   >> /etc/bash.bashrc
RUN mkdir /src && echo PS1=\"\\h:\\w\\$ \"                    >> /etc/bash.bashrc
RUN echo cp /src/exmips /                                     >> /etc/bash.bashrc
RUN echo "if [ ! -f /firm_docker.bin ]; then"                 >> /etc/bash.bashrc && \
    echo "   cp /src/firm_docker.bin /"                       >> /etc/bash.bashrc && \
    echo "fi"                                                 >> /etc/bash.bashrc && \
    echo ""                                                   >> /etc/bash.bashrc
RUN echo /exmips -tap tap0 -f 256 -s firm_docker.bin          >> /etc/bash.bashrc
ENTRYPOINT ["bash"]