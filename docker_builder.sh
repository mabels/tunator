#!/bin/bash
#/etc/init.d/docker start
(
  cd /tunator-build 
  for i in mabels/tunator.git \
      eidheim/Simple-WebSocket-Server.git \
      mabels/easyloggingpp.git \
      mabels/ipaddress.git \
      open-source-parsers/jsoncpp.git \
      mabels/tunator.git 
  do
    dir=$(dirname $i)
    echo $dir "::::" $i
    rm -rf $i
    (mkdir -p $dir ; cd $dir ; git clone --bare https://github.com/$i )
  done
)

for arch in multiarch/ubuntu-core:armhf-xenial multiarch/ubuntu-core:arm64-xenial ubuntu:16.04 
do
  (
  farch=$(echo $arch | sed 's/[^a-zA-Z0-9]/-/g')
  mkdir build-$farch
  cd build-$farch
  rm -f .dockerignore
  cat > docker_compile.sh <<EOF
apt-get update
apt-get upgrade -y
apt-get install -y -q git cmake make g++ libssl-dev libboost-all-dev docker.io
#apt-get install -y -q git 
git clone /tunator-build/mabels/tunator.git
cd tunator && \
  cmake -DGITBASE=/tunator-build/ -DCMAKE_BUILD_TYPE=Release . && \
  make 
#&& \
#  ctest --timeout 60 && \
#  test/Release/tuna_server_test
  mkdir -p /tunator-build/$farch 
  git rev-parse --verify --short HEAD > /tunator-build/$farch/VERSION
  cp Release/*/main /tunator-build/$farch/tunator
  #touch /tunator-build/$farch/tunator
EOF
#  cat > /bin/sh.arm << EOF
#!/usr/bin/qemu-arm-static /bin/sh.real
#et -o errexit
#p /bin/sh.real /bin/sh  
#bin/sh "$@"
#p /usr/bin/sh-shim /bin/sh 
#OF
#if [ "$arch" = "ubuntu:16:04" ]
#then
  echo "Setup $arch"
  EUM=""
  CMD="CMD [\"/bin/bash\", \"/docker_compile.sh\"]"
  COPY=""
  EMUVERSION=""
  APT="\"apt\""
  DIGN=""
#fi
if [ $arch = "multiarch/ubuntu-core:armhf-xenial" ]
then
  EMU="/usr/bin/qemu-arm-static"
  CMD="CMD [\"$EMU\", \"/bin/bash\", \"/docker_compile.sh\"]"
  APT="\"$EMU\", \"/usr/bin/apt\""
  COPY="COPY $(basename $EMU) $EMU"
  EMUVERSION="RUN $EMU --version"
  #COPY=""
  DIGN="!$(basename $EMU)"
  #mount binfmt_misc -t binfmt_misc /proc/sys/fs/binfmt_misc
  #echo -1 > /proc/sys/fs/binfmt_misc/arm
  #echo ':arm:M::\x7fELF\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x28\x00:\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff:/usr/bin/qemu-arm-static:' > /proc/sys/fs/binfmt_misc/register
#  cp $EMU .
fi
if [ $arch = "multiarch/ubuntu-core:arm64-xenial" ]
then
  EMU="/usr/bin/qemu-aarch64-static"
  CMD="CMD [\"$EMU\", \"/bin/bash\", \"/docker_compile.sh\"]"
  COPY="COPY $(basename $EMU) $EMU"
  EMUVERSION="RUN $EMU --version"
  #COPY=""
  APT="\"$EMU\", \"/usr/bin/apt\""
  DIGN="!$(basename $EMU)"
  #mount binfmt_misc -t binfmt_misc /proc/sys/fs/binfmt_misc
  #echo -1 > /proc/sys/fs/binfmt_misc/aarch64
  #echo ':aarch64:M::\x7fELF\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x28\x00:\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff:/usr/bin/qemu-aarch64-static:' > /proc/sys/fs/binfmt_misc/register
#      k  :aarch64:M::\x7fELF\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x28\x00:\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff:/qemu/qemu-aarch64-binfmt:P
#  cp $EMU .
fi

  cat > Dockerfile <<EOF
FROM $arch
$COPY
$EMUVERSION
COPY docker_compile.sh /
$CMD
EOF
echo "builder for tunator-builder-$farch $arch-"
cat Dockerfile
#read a
  cp $EMU .
  docker build -t tunator-builder-$farch .
echo "run for tunator-builder-$farch"
#read a
  docker run -v /tunator-build:/tunator-build -ti tunator-builder-$farch 
echo "done for builder-$farch"
#read a

cp /tunator-build/$farch/tunator .
cp /tunator-build/$farch/VERSION .
pwd
ls -la
cat > Dockerfile <<RUNNER
FROM $arch
$COPY
$EMUVERSION
RUN [$APT, "update", "-y"]
RUN [$APT, "upgrade", "-y"]
RUN [$APT, "install", "-y", "-q", "libssl1.0.0", "libboost-system1.58.0", "libboost-regex1.58.0"]
COPY tunator /
COPY VERSION /

CMD ["/tunator"]
RUNNER

cat > .dockerignore <<RUNNER
*
!tunator
!VERSION
$DIGN
RUNNER

mkdir -p $HOME/.docker
cat > $HOME/.docker/config.json <<RUNNER
{
	"auths": {
		"https://index.docker.io/v1/": {
			"auth": "$DOCKER_AUTH"
		}
	}
}
RUNNER
echo "done for tunator-builder-$farch"
cat Dockerfile
#read a
cp $EMU .
docker build -t tunator-$farch .
echo "done for tunator-$farch"
#read a
TVERSION=$farch-$(cat /tunator-build/$farch/VERSION)
docker images
echo docker tag tunator-$farch fastandfearless/tunator:$TVERSION
docker tag tunator-$farch fastandfearless/tunator:$TVERSION
echo "done for tunator-$farch"
#read a
echo docker push fastandfearless/tunator:$TVERSION
docker push fastandfearless/tunator:$TVERSION
echo "done for tunator-$farch"
#read a
)
done
