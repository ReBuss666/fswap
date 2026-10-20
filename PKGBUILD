# Maintainer: Adilet <deathnewacc@gmail.com>
pkgname=fswap
pkgver=0.1.0
pkgrel=1
pkgdesc="Fast atomic and content file swapping utility for Linux"
arch=('x86_64')
url="https://github.com/YOUR_GITHUB_USERNAME/fswap"
license=('MIT')
depends=('glibc' 'gcc-libs')
makedepends=('make' 'gcc')
source=("$pkgname-$pkgver.tar.gz::$url/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
  cd "$pkgname-$pkgver"
  make
}

package() {
  cd "$pkgname-$pkgver"
  make PREFIX=/usr DESTDIR="$pkgdir" install
}
