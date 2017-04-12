/*Chassis beta 
this version have support deamon to deamon handshake, only give real information to tcp client (diag main app or chassis app)

you can use chassis app to send group command, or use chassis_client app to send single card's command.
*/

/*chassis use tcp port:64507 and udp port:64507 To disable*/
iptables -A INPUT -p tcp --dport 64507 -j ACCEPT
iptables -A INPUT -p udp --dport 64507 -j ACCEPT



compile code:
/*date time need to change, let it bigger than now*/
date -s "20150417 20:18:00

/*clean*/
make clean : 

/*make code*/
make 

/* install */
/*copy files to specify directory*/
make install

/*it shall copy below files*/
chassis    /usr/local/accton/bin/chassis  : daemon app
chassisd   /usr/local/accton/bin/chassid : daemon start file
chassis.conf /usr/local/accton/bin/chassis.conf : daemon config file
us_card_number /usr/local/accton/bin/us_card_number : daemon keep slot id
chassis_accton.conf: reserve file
chassis_facebook.conf: reserve file

/spi_user/bcm5396 /sbin/bcm5396 : support spi api for manage switch 5396 & 5389
/udp_client/chassis_client : app to control deamon
/tcp_client/chassis_agent: reserved, not support this version.

/*uninstall*/
/*remove below files*/
chassis    /usr/local/accton/bin/chassis
chassisd   /usr/local/accton/bin/chassid
chassis.conf /usr/local/accton/bin/chassis.conf
/spi_user/bcm5396 /sbin/bcm5396
/udp_client/chassis_client /usr/local/accton/bin/chassis_client
/tcp_client/chassis_agent /usr/local/accton/bin/chassis_agent


command:
/*operation for chassis deamon*/

chassisd start
chassisd stop
chassisd restart
chassisd status


chassis_client 127.0.0.1 1 : get chassis information


