pkgname=kitten
pkgver=0.1.0
pkgrel=1
pkgdesc="Readable source-tree snapshots for the terminal"
arch=('x86_64')
url="https://github.com/reqsd64/kitten"
license=('BSD-2-Clause')
depends=()
makedepends=('make' 'gcc')

source=("$pkgname-$pkgver.tar.gz::$url/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('74dd95db270fa1e85509247e26e702a94b373b3982051720ba4aec68dae2a083')

build() {
    cd "$srcdir/$pkgname-$pkgver"
    make
}

package() {
    cd "$srcdir/$pkgname-$pkgver"

    make PREFIX=/usr DESTDIR="$pkgdir" install
    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
