#!/bin/bash

PREFIX_DIR="/usr/local"
INSTALL_SHAREDIR="${PREFIX_DIR}/share/codeblocks"
APPNAME="CodeBlocks"
APPDIR="${APPNAME}.app"
SHAREDATADIR="`pwd`/${APPDIR}/Contents/Resources/share/codeblocks"

if [ ! -d "$APPDIR" ]
then
    echo "Building ${APPDIR} directory..."
    mkdir "${APPDIR}"
    mkdir "${APPDIR}/Contents"
    mkdir "${APPDIR}/Contents/MacOS"
    mkdir "${APPDIR}/Contents/Resources"
    mkdir -p "${SHAREDATADIR}/plugins/"
fi

cp ${INSTALL_SHAREDIR}/osx_bundle/codeblocks.plist "$APPDIR/Contents/Info.plist"
cp ${INSTALL_SHAREDIR}/osx_bundle/icons/*.icns "${APPDIR}/Contents/Resources"


PLUGINS=`echo ${PREFIX_DIR}/lib/codeblocks/plugins/*.dylib `
EXECUTABLES=`echo ${PREFIX_DIR}/bin/* `

ALL_DEPS=""

copy_deps() {
	for dep in `otool -L "$1" | sed -n 's/\(.*[^\\ ]\) (.*/\1/gp'`
	do
		current_dep=$(basename $dep)
		if [[ $dep =~ libcodeblocks ]]
		then
			install_name_tool -change $dep @executable_path/$(basename $dep) $1
		fi
		if [[ $dep =~ libwx ]]
		then
			oldfile=$dep
			oldlink=$dep
			newfile=$dep
			while [ -L $newfile ]
			do
				newfile=`readlink $newfile`
				if ! echo $newfile | grep '^/'
				then
					newfile=$(dirname $oldfile)/$newfile
				fi
				oldfile=$newfile
			done
			dep=$newfile
			current_dep=$(basename $dep)
			install_name_tool -change $oldlink "@executable_path/$current_dep" $1
			if [[ ! $ALL_DEPS =~ $current_dep ]]
			then
				ALL_DEPS="$ALL_DEPS $current_dep"
				echo "Copying $dep"
				cp "$dep" "$APPDIR/Contents/MacOS/"
				install_name_tool -id "@executable_path/$current_dep" "$APPDIR/Contents/MacOS/$current_dep"
				copy_deps "$APPDIR/Contents/MacOS/$current_dep"
			fi
		fi
	done
}

echo "Copying libcodeblocks ..."
cp -v ${PREFIX_DIR}/lib/libcodeblocks.0.dylib "$APPDIR/Contents/MacOS/"
copy_deps "${APPDIR}/Contents/MacOS/libcodeblocks.0.dylib"
install_name_tool -id "@executable_path/libcodeblocks.0.dylib" "$APPDIR/Contents/MacOS/libcodeblocks.0.dylib"

echo "Copying binaries"
for file in ${EXECUTABLES}
do
	echo $file
	cp $file "${APPDIR}/Contents/MacOS"
	copy_deps "${APPDIR}/Contents/MacOS/$(basename $file)"
done

echo "Copying plugins"
for file in ${PLUGINS}
do
	echo $file
	cp $file "${APPDIR}/Contents/Resources/share/codeblocks/plugins/"
	install_name_tool -id @loader_path/$(basename $file) "${SHAREDATADIR}/plugins/$(basename $file)"
	copy_deps "${SHAREDATADIR}/plugins/$(basename $file)"
done

rsync -lpdtgou --include='*.zip' --exclude='*' ${INSTALL_SHAREDIR}/ ${SHAREDATADIR}/
rsync -rlpdtgou  ${INSTALL_SHAREDIR}/images ${SHAREDATADIR}/
rsync -rlpdtgou  ${INSTALL_SHAREDIR}/templates ${SHAREDATADIR}/
rsync -rlpdtgou  ${INSTALL_SHAREDIR}/lexers ${SHAREDATADIR}/
rsync -rlpdtgou  ${INSTALL_SHAREDIR}/scripts ${SHAREDATADIR}/
rsync -rlpdtgou  ${INSTALL_SHAREDIR}/compilers ${SHAREDATADIR}/
rsync -rlpdtgou  ${INSTALL_SHAREDIR}/SpellChecker ${SHAREDATADIR}/
