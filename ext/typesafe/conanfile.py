from conans import ConanFile
from conans.tools import download, untargz
import os

class TypeSafe(ConanFile):
    name = 'type_safe'
    url  = 'https://github.com/foonathan/type_safe'
    version = '0.2.4'
    requires = 'debug_assert/1.3@Manu343726/testing'
    exports = '*.hpp'
    generators = 'cmake'


    def source(self):
        tar = 'type_safe-{}.tar.gz'.format(self.version)
        url = 'https://github.com/foonathan/type_safe/archive/v{}.tar.gz'.format(self.version)
        download(url, tar)
        untargz(tar)

    def package(self):
        includedir = os.path.join('include', 'type_safe')
        srcdir = os.path.join('type_safe-{}'.format(self.version), includedir)
        self.copy('*.hpp', src=srcdir, dst=includedir)
