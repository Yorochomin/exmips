#
# Network setting of OpenWrt in a container environment
# 
uci del network.@switch_vlan[0]
uci del network.@switch_vlan[0]
uci del network.@switch[0]
uci del network.@device[0]
uci del network.@device[0]
uci del network.lan
uci del network.wan
uci del network.wan6
uci set network.wan=interface
uci set network.wan.device='eth0'
uci set network.wan.proto='static'
uci set network.wan.ipaddr='192.168.100.10'
uci set network.wan.netmask='255.255.255.0'
uci set network.wan.gateway='192.168.100.1'
uci set network.wan.dns='8.8.8.8'
uci commit network
/etc/init.d/network reload 
killall ntpd

