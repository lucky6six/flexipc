rm -rf build
#sh ./docker_build.sh -DCHCORE_PLAT=hikey970
sh ./docker_fastbuild.sh -DCHCORE_PLAT=hikey970
cd ./tools/hikey970
sudo sh ./build_rootfs.sh
sudo fastboot flash system ./rootfs.img
