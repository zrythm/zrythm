# Maintainer: Alexandros Theodotou <alex at alextee dot online>

pkgname=zrythm-git
pkgver=0.1
pkgrel=1
pkgdesc="Free GNU/Linux music production system (DAW)"
arch=('x86_64')
url="https://gitlab.com/alextee/zrythm"
license=('GPL')
depends=('gtk3' 'lv2' 'lilv-git' 'suil-git' 'jack' 'libsndfile' 'libsmf')
makedepends=('git')
source=(sources.tar.gz)
md5sums=('SKIP')

build() {
  ./autogen.sh
	./configure --prefix=/usr --enable-aur-build
	make -j14
}

package() {
	make DESTDIR="$pkgdir/" install
}

