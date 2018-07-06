#!/bin/bash

#read -p "Username >" username
#IFS= read -p "Password >" password
username="tony"
password="xxxxx"
salt=$(sudo getent shadow $username | cut -d$ -f3)
epassword=$(sudo getent shadow $username | cut -d: -f2)
match=$(python -c 'import crypt; print crypt.crypt("'"${password}"'", "$6$'${salt}'")')
[ ${match} == ${epassword} ] && echo "Password matches" || echo "Password doesn't match"


if printf "%s\0" "$password" | /sbin/unix_chkpwd "$USER" nullok; then
    echo "password match"
else
    echo "ERROR: Your login passwd is incorrect"
fi
