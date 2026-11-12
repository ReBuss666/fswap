# Maintainer: Adilet <deathnewacc@gmail.com>
pkgname=fswap
pkgver=1.0.0
pkgrel=1
pkgdesc="Fast atomic and content file swapping utility for Linux"
arch=('x86_64')
url="https://github.com/ReBuss666/fswap"
license=('MIT')
depends=('glibc' 'gcc-libs')
makedepends=('make' 'gcc')
optdepends=(
  'bash-completion: command auto-completion support for Bash'
  'zsh: command auto-completion support for Zsh'
)
source=("$pkgname-$pkgver.tar.gz::$url/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('1da285b0d1928ef0d898e4afc62f40c84217644ea0fc9fbbeb5e1dff8aa2cb39')

build() {
  cd "$pkgname-$pkgver"
  make
}

package() {
  cd "$pkgname-$pkgver"
  make PREFIX=/usr DESTDIR="$pkgdir" install
}
