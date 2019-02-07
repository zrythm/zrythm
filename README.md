Zrythm is a free modern music production system, also known as DAW.
It aims to be highly modular, meaning everything should be automatable and inter-connectable, making it suitable for electronic music.

It is written in C and it uses the GTK+3 toolkit. Bits and pieces are taken from other programs like Ardour and Jalv.

More info at https://www.zrythm.org

## Current state
![screenshot 1](https://www.zrythm.org/img/Screenshot_20190105_165238.png)
![screenshot 2](https://www.zrythm.org/img/Screenshot_20190105_165002.png)

## Currently supported protocols:
- LV2

## Installation
### Arch Linux/Manjaro
Substitute `yaourt` with your AUR-compatible package manager
```
# stable
yaourt -S zrythm

# latest (unstable)
yaourt -S zrythm-git
```
### Debian/Linux Mint/Ubuntu
add the public key
```
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 1D597D35
```
add the appropriate repository
```
# for Ubuntu 18.04 LTS/Linux Mint 19
sudo apt-add-repository "deb https://apt.alextee.org/ubuntu bionic main"

# for Ubuntu 18.10
sudo apt-add-repository "deb https://apt.alextee.org/ubuntu cosmic main"

# for Debian Buster/Devuan Beowulf
sudo apt-add-repository "deb https://apt.alextee.org/debian buster main"
```

install
```
sudo apt-get update && sudo apt-get install zrythm
```

_Note: If you don't have apt-add-repository, install it using_
`
sudo apt-get install software-properties-common
`

_Note2: If some dependencies are not found, you might need to enable the universe repository (before installing) using_
`
sudo apt-add-repository universe
`

_Note3: For latest (unstable) version, install this directly by double clicking or by using `sudo dpkg -i` and then `sudo apt-get -f install`:_ https://git.zrythm.org/zrythm/zrythm/-/jobs/artifacts/master/download?job=build_deb_64_unstable
### Fedora 27/28/29
enable dnf repository
```
sudo dnf -y copr enable alextee/zrythm
```
install
```
sudo dnf -y install zrythm
```
### Manual installation
_Note: You will need to have the development libraries for libxml2, gtk3, jack, lilv, suil, libsmf, libsndfile, libdazzle and lv2 installed_
```
git clone https://git.zrythm.org/zrythm/zrythm.git
cd zrythm
./autogen.sh
./configure
make
sudo make install
```

## Using
At the moment, Zrythm assumes you have Jack installed and will only run if Jack is running. For Jack setup instructions see https://libremusicproduction.com/articles/demystifying-jack-%E2%80%93-beginners-guide-getting-started-jack

Please refer to the [manual](https://manual.zrythm.org) for more information.

## Contributing
For detailed installation instructions see [INSTALL.md](INSTALL.md)

For contributing guidelines see [CONTRIBUTING.md](CONTRIBUTING.md)

For any bugs please raise an issue or join a chatroom below

## Chatrooms
### Matrix/IRC (Freenode)
`#zrythm` channel (for Matrix users `#freenode_#zrythm:matrix.org`)

## License
Copyright (C) 2018-2019  Alexandros Theodotou

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

## Support
Donations of any amount are accepted (and appreciated) below:

### Paypal
https://www.paypal.me/alextee90
### LiberaPay
https://liberapay.com/alextee/
### Bitcoin
coming soon
