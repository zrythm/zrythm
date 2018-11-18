# Maintainer: Alexandros Theodotou <alex at alextee dot online>

pkgname=zrythm-git
pkgver=0.1
pkgrel=1
pkgdesc="Free GNU/Linux music production system (DAW)"
arch=('x86_64')
url="https://gitlab.com/alextee/zrythm"
license=('GPL')
depends=('gtk3' 'lilv-git' 'suil-git' 'jack2' 'libsndfile' 'libsmf')
makedepends=('git')
source=("$pkgname-$pkgver::git+https://gitlab.com/alextee/zrythm.git")
md5sums=('SKIP')

prepare() {
	cd "$pkgname-$pkgver"
}

build() {
	cd "$pkgname-$pkgver"
	./configure --prefix=/usr --enable-aur-build
	make
}

package() {
	cd "$pkgname-$pkgver"
	make DESTDIR="$pkgdir/" install
}

