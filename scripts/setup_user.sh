# Create User
echo "creating token user....";
useradd token
passwd token
adduser token syslog

# Create Logging Directory
echo "creating token logs directory....";
mkdir -p /var/log/token/
chown token:token /var/log/token

# Create Ledger Directory
echo "creating token ledger directory....";
mkdir -p /opt/token/ledger
chown token:token /opt/token/ledger