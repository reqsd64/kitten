pkgname=kitten
pkgver=1.0.0
pkgrel=1
pkgdesc="Kitten application"
arch=('x86_64')
url="https://github.com/USERNAME/kitten"
license=('MIT')
depends=()
makedepends=('make' 'gcc')

source=("$pkgname-$pkgver.tar.gz::$url/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cd "$srcdir/$pkgname-$pkgver"
    make
}

package() {
    cd "$srcdir/$pkgname-$pkgver"

    install -Dm755 kitten "$pkgdir/usr/bin/kitten"
    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}