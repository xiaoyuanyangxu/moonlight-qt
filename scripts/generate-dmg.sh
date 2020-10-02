# This script requires create-dmg to be installed from https://github.com/sindresorhus/create-dmg
BUILD_CONFIG=$1

fail()
{
	echo "$1" 1>&2
	exit 1
}

if [ "$BUILD_CONFIG" != "Debug" ] && [ "$BUILD_CONFIG" != "Release" ]; then
  fail "Invalid build configuration - expected 'Debug' or 'Release'"
fi

BUILD_ROOT=$PWD/build
SOURCE_ROOT=$PWD
BUILD_FOLDER=$BUILD_ROOT/build-$BUILD_CONFIG
INSTALLER_FOLDER=$BUILD_ROOT/installer-$BUILD_CONFIG
VERSION=`cat $SOURCE_ROOT/app/version.txt`

if [ "$SIGNING_PROVIDER_SHORTNAME" == "" ]; then
  SIGNING_PROVIDER_SHORTNAME=$SIGNING_IDENTITY
fi
if [ "$SIGNING_IDENTITY" == "" ]; then
  SIGNING_IDENTITY=$SIGNING_PROVIDER_SHORTNAME
fi

[ "$SIGNING_IDENTITY" == "" ] || git diff-index --quiet HEAD -- || fail "Signed release builds must not have unstaged changes!"

echo Cleaning output directories
rm -rf $BUILD_FOLDER
rm -rf $INSTALLER_FOLDER
mkdir $BUILD_ROOT
mkdir $BUILD_FOLDER
mkdir $INSTALLER_FOLDER

echo Configuring the project
pushd $BUILD_FOLDER
qmake $SOURCE_ROOT/moonlight-qt.pro || fail "Qmake failed!"
popd

echo Compiling LudicoEdge in $BUILD_CONFIG configuration
pushd $BUILD_FOLDER
make -j$(sysctl -n hw.logicalcpu) $(echo "$BUILD_CONFIG" | tr '[:upper:]' '[:lower:]') || fail "Make failed!"
popd

echo Saving dSYM file
pushd $BUILD_FOLDER
dsymutil app/LudicoEdge.app/Contents/MacOS/LudicoEdge -o LudicoEdge-$VERSION.dsym || fail "dSYM creation failed!"
cp -R LudicoEdge-$VERSION.dsym $INSTALLER_FOLDER || fail "dSYM copy failed!"
popd

echo Creating app bundle
EXTRA_ARGS=
if [ "$BUILD_CONFIG" == "Debug" ]; then EXTRA_ARGS="$EXTRA_ARGS -use-debug-libs"; fi
echo Extra deployment arguments: $EXTRA_ARGS
macdeployqt $BUILD_FOLDER/app/LudicoEdge.app $EXTRA_ARGS -qmldir=$SOURCE_ROOT/app/gui -appstore-compliant || fail "macdeployqt failed!"

if [ "$SIGNING_IDENTITY" != "" ]; then
  echo Signing app bundle
  codesign --force --deep --options runtime --timestamp --sign "$SIGNING_IDENTITY" $BUILD_FOLDER/app/LudicoEdge.app || fail "Signing failed!"
fi

echo Creating DMG
echo $INSTALLER_FOLDER
echo $SIGNING_IDENTITY
echo $BUILD_FOLDER
if [ "$SIGNING_IDENTITY" != "" ]; then
  create-dmg $BUILD_FOLDER/app/LudicoEdge.app $INSTALLER_FOLDER --identity="$SIGNING_IDENTITY" || fail "create-dmg failed!"
else
  create-dmg $BUILD_FOLDER/app/LudicoEdge.app $INSTALLER_FOLDER
  case $? in
    0) ;;
    2) ;;
    *) fail "create-dmg failed!";;
  esac
fi

if [ "$NOTARY_USERNAME" != "" ] && [ "$NOTARY_PASSWORD" != "" ]; then
  echo Uploading to App Notary service
  xcrun altool -t osx -f $INSTALLER_FOLDER/LudicoEdge\ $VERSION.dmg --primary-bundle-id com.ludico_edge.LudicoEdge --notarize-app --username "$NOTARY_USERNAME" --password "$NOTARY_PASSWORD" --asc-provider "$SIGNING_PROVIDER_SHORTNAME" || fail "Notary submission failed"
  
  echo Waiting 5 minutes for notarization to complete
  sleep 300

  echo Getting notarization status
  xcrun altool -t osx --notarization-history 0 --username "$NOTARY_USERNAME" --password "$NOTARY_PASSWORD" --asc-provider "$SIGNING_PROVIDER_SHORTNAME" || fail "Unable to fetch notarization history!"

  echo Stapling notary ticket to DMG
  xcrun stapler staple -v $INSTALLER_FOLDER/LudicoEdge\ $VERSION.dmg || fail "Notary ticket stapling failed!"
fi

mv $INSTALLER_FOLDER/LudicoEdge\ $VERSION.dmg $INSTALLER_FOLDER/LudicoEdge-$VERSION.dmg
echo Build successful
