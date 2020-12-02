FROM debian:buster as build

LABEL description="Build container - mibridge"

RUN apt-get update && apt-get install -y \
    git build-essential cmake

RUN cd /tmp \
    && git clone https://github.com/jswquizmaster/librf24-bcm -n \
    && cd librf24-bcm \
    && git checkout 3f970fd64b9b8ec0f369cf772ca25965281949c1 \
    && make \
    && make install

ADD . /mibridge
WORKDIR /mibridge

RUN mkdir build \
    && cd build \
    && cmake .. \
    && make

FROM debian:buster as runtime

LABEL description="Run container - mibridge"

COPY --from=build /usr/local/lib/librf24* /usr/local/lib/

COPY --from=build /mibridge/build/mibridge /

CMD ["/mibridge"]
