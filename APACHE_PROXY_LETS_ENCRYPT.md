#Apache Proxy, LetsEncrypt & CertBot

####Install Apache

`$ apt-get install apache2`

####Enable proxy

`$ a2enmod proxy && sudo a2enmod proxy_http && a2enmod headers`

* Setup a standard apache v-host proxy on port 80 pointing to port 8181:

`$ vim /etc/apache2/sites-available/willkamp.conf`

```
"<VirtualHost *:80>
   ServerName willkamp.com
   ServerAlias 173.230.147.167
   ServerAlias www.willkamp.com
   ProxyRequests Off
   <Proxy *>
       Order deny,allow
       Allow from all
   </Proxy>
   ProxyPass / http://127.0.0.1:8181/
   ProxyPassReverse / http://127.0.0.1:8181/
   <Location / >
       ProxyPassReverse /
       Order deny,allow
       Allow from all
   </Location>
</VirtualHost>"
```

`$ a2ensite weiilkamp`

####Get [LetsEncrypt](https://letsencrypt.org/)
```
cd /opt

git clone https://github.com/letsencrypt/letsencrypt.git

```

####Install LetsEncrypt for the v-host
```
./letsencrypt-auto --apache -d willkamp.com -d www.willkamp.com
```

####Setup [CertBot (LetsEncrypt)](https://certbot.eff.org/) certificate auto-renewal
```
wget https://dl.eff.org/certbot-auto
chmod a+x certbot-auto

$crontab -3
30 2 * * 1 .path/to/certbot-auto renew >> /var/log/certbot-renew.log
```


