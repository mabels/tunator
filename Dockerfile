FROM ubuntu:16.04

RUN apt update -y
#RUN apt install -y -q git cmake make g++ libssl-dev libboost-all-dev docker.io qemu-user-static
RUN apt install -y -q docker.io qemu-user-static 
#qemu-user-static
COPY docker_builder.sh /my_app/

CMD ["/bin/bash", "/my_app/docker_builder.sh"]
