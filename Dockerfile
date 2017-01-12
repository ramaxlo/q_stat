FROM ubuntu:14.04

MAINTAINER Ramax Lo ramax.lo@gmail.com

RUN apt-get update
RUN apt-get -y install \
            libavcodec54 \
            libavutil52 \
            libavformat54

COPY q_stat /usr/bin

WORKDIR /root
ENTRYPOINT ["/usr/bin/q_stat"]
