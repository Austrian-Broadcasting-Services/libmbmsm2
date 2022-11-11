# libmbmsm2
M2 ASN.1 C++ interface code

## Dependencies

````
sudo apt install libusrsctp-dev flex

````

## Compile and install APER-capable ASN.1 compiler

git clone https://github.com/mouse07410/asn1c.git
cd asn1c/
test -f configure || autoreconf -iv
./configure
make
sudo make install

