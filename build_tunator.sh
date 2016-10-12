
echo $DOCKER_HOST
auth=$(ruby -e 'require "json"; puts JSON.parse(IO.read("#{ENV["HOME"]}/.docker/config.json"))["auths"]["https://index.docker.io/v1/"]["auth"]')
docker build -t build-tunator .
docker run -ti --rm --privileged multiarch/qemu-user-static:register --reset
docker run -d -v /var/run/docker.sock:/var/run/docker.sock -v /tunator-build:/tunator-build  --privileged --env "DOCKER_AUTH=$auth" -t build-tunator
#/bin/bash
