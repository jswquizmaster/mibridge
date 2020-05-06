FROM alpine:3.11 as build

LABEL description="Build container - mibridge"

RUN apk update && apk add --no-cache \ 
    build-base binutils cmake curl file gcc g++ git libgcc libtool linux-headers make musl-dev tar unzip wget

RUN cd /tmp \
    && git clone https://github.com/jswquizmaster/RF24 -n \
    && cd RF24 \
    && git checkout 17e3eb12f772744f9376df2f31e83e55c1709471 \
    && ./configure --driver=SPIDEV --ldconfig='' \
    && make \
    && make install

ADD . /mibridge
WORKDIR /mibridge

RUN mkdir build \
    && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release .. \
    && make

FROM alpine:3.11 as runtime

LABEL description="Run container - mibridge"

RUN apk update && apk add --no-cache \ 
    libgcc libstdc++

COPY --from=build /usr/local/lib/librf24* /usr/local/lib/

COPY --from=build /mibridge/build/mibridge /

CMD ["/mibridge"]
