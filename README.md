Zrythm is a free modern music production system, also known as DAW.
It aims to be highly modular, meaning everything should be automatable and inter-connectable, making it suitable for electronic music.

It is written in C and it uses the GTK+3 toolkit. Bits and pieces are taken from other programs like Ardour and Jalv.

More info at https://www.zrythm.org

## Current state
![screenshot](https://alextee.website/wp-content/uploads/2018/12/Screenshot_20181230_011119.png)
![screenshot](https://alextee.website/wp-content/uploads/2018/11/Screenshot_20181102_141207.png)

## Currently supported protocols:
- LV2

## Installation
### Arch Linux/Manjaro
```
# yaourt
yaourt -S zrythm-git

# yay
yay -S zrythm-git
```
### Debian/Linux Mint/Ubuntu
```
# add the repository
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 1D597D35
sudo apt-add-repository "deb http://apt.alextee.org/apt/debian buster main"

# install
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
### Fedora
coming soon
### Manual installation
_Note: You will need to have the development libraries for libxml2, gtk3, jack, lilv, suil, libsmf, libsndfile, libdazzle and lv2 installed_
```
git clone https://gitlab.com/alextee/zrythm.git
cd zrythm
./autogen.sh
./configure
make
sudo make install
```

## Using
At the moment, Zrythm assumes you have Jack installed and will only run if Jack is running. Everything else should be self-explanatory.

## Contributing
For detailed installation instructions see INSTALL

For contributing guidelines see CONTRIBUTING

For any bugs please raise an issue or join a chatroom below

## Chatrooms
### IRC (Freenode)
`#zrythm` channel
### Matrix
`zrythm` room (https://matrix.to/#/!aPVJxKGBLLAIRygQYD:matrix.org)
### Discord
`zrythm` server (https://discord.gg/AbHb3eD)

## License
GPLv3+

## Support
Monthly donations of any amount are accepted (and appreciated) below:

### Patreon
https://www.patreon.com/alextee
### Liberapay
https://liberapay.com/alextee/
### Paypal
https://www.paypal.me/alextee90
