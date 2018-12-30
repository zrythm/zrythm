# Maintainer: Alexandros Theodotou <alex at alextee dot online>

pkgname=zrythm-git
pkgver=0.1
pkgrel=1
pkgdesc="Free GNU/Linux music production system (DAW)"
arch=('x86_64')
url="https://gitlab.com/alextee/zrythm"
license=('GPL')
depends=('gtk3' 'lv2' 'lilv' 'suil' 'jack' 'libsndfile' 'libsmf')
source=(sources.tar.gz)
md5sums=('SKIP')

build() {
  ./autogen.sh
	./configure --prefix=/usr --enable-release --enable-aur-build
	make -j14
}

package() {
	make DESTDIR="$pkgdir/" install
}
