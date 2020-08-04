
apt-get install xz-utils
apt-get install libusb-dev libdbus-1-dev libglib2.0-dev libudev-dev libical-dev libreadline-dev
apt-get install build-essential
mkdir ~/download
cd ~/download
wget http://www.kernel.org/pub/linux/bluetooth/bluez-5.44.tar.xz
tar xvf bluez-5.44.tar.xz
cd bluez-5.44
./configure --enable-library
make
make install

apt-get install pulseaudio pulseaudio-module-bluetooth
systemctl --global disable pulseaudio.service pulseaudio.socket
adduser pulse audio
adduser pulse bluetooth
adduser root pulse-access
apt-get install dbus

apt-get install python3-pip

curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=/usr/local/bin sh
arduino-cli config init
arduino-cli core update-index
arduino-cli install arduino:avr
arduino-cli lib update-index
arduino-cli lib install "Time"
arduino-cli lib install "ArduinoJson"
cd ~/Arduino/libraries
git clone https://github.com/SpellFoundry/PCF8523.git
git clone https://github.com/rocketscream/Low-Power.git

apt-get install avrdude
wget https://github.com/SpellFoundry/avrdude-rpi/archive/master.zip
unzip master.zip
cd ./avrdude-rpi-master/
cp autoreset /usr/bin
cp avrdude-autoreset /usr/bin
mv /usr/bin/avrdude /usr/bin/avrdude-original
ln -s /usr/bin/avrdude-autoreset /usr/bin/avrdude

apt-get install git
git config --global user.name "Jim Dickinson"
git config --global user.email "james.n.dickinson@gmail.com"
git clone https://github.com/jimbroze/RetroPlayer.git

apt-get install i2c-tools

apt-get install pigpiod
apt-get install libgirepository1.0-dev gcc libcairo2-dev pkg-config python3-dev gir1.2-gtk-3.0
pip3 install python-dbus
pip3 install asyncio_glib
pip3 install pigpio
pip3 install pyserial-asyncio
apt-get install python-dev python-rpi.gpio

pip3 install adafruit-blinka
pip3 install pillow
pip3 install adafruit-circuitpython-ssd1305