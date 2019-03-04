
echo -e "\nls $TARBALL_FILE"
           ls $TARBALL_FILE            || exit 1
echo -e "\nls ./$TARBALL_FILENAME"
           ls ./$TARBALL_FILENAME      || exit 1
echo -e "\nls $TARBALL_FILE.sig"
           ls $TARBALL_FILE.sig        || exit 1
echo -e "\nls ./$TARBALL_FILENAME.sig"
           ls ./$TARBALL_FILENAME.sig  || exit 1
echo -e "\nls $PKGBUILD_FILE"
           ls $PKGBUILD_FILE           || exit 1
echo -e "\nls ./PKGBUILD"
           ls ./PKGBUILD               || exit 1
echo -e "\nls $PKGBUILD_FILE.sig"
           ls $PKGBUILD_FILE.sig       || exit 1
echo -e "\nls ./PKGBUILD.sig"
           ls ./PKGBUILD.sig           || exit 1

for remote_file in ${REMOTE_FILES[@]}
do  [ -f $remote_file -a "$(stat --printf='%s' $remote_file)" != "9" ] && does_remote_exist=1 || does_remote_exist=0
    echo -e "\n$remote_file does_remote_exist=$does_remote_exist" ; (($does_remote_exist)) || exit 1 ;
done

echo -e "\ngpg --verify --verify-options no-show-photos ./$TARBALL_FILENAME.sig"
           gpg --verify --verify-options no-show-photos ./$TARBALL_FILENAME.sig  || exit 1
echo -e "\ndiff $TARBALL_FILE ./$TARBALL_FILENAME"
           diff $TARBALL_FILE ./$TARBALL_FILENAME                                || exit 1
echo -e "\ndiff $TARBALL_FILE.sig ./$TARBALL_FILENAME.sig"
           diff $TARBALL_FILE.sig ./$TARBALL_FILENAME.sig                        || exit 1
echo -e "\ngpg --verify --verify-options no-show-photos ./PKGBUILD.sig"
           gpg --verify --verify-options no-show-photos ./PKGBUILD.sig           || exit 1
echo -e "\ndiff $PKGBUILD_FILE ./PKGBUILD"
           diff $PKGBUILD_FILE ./PKGBUILD                                        || exit 1
echo -e "\ndiff $PKGBUILD_FILE.sig ./PKGBUILD.sig"
           diff $PKGBUILD_FILE.sig ./PKGBUILD.sig                                || exit 1
