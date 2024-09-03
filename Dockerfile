ARG DOCKER_IMAGE=alpine:3.19
FROM $DOCKER_IMAGE AS dev

ENV LUAJIT_VERSION v2.1

RUN apk add --no-cache git build-base cmake curl-dev zlib-dev zstd-dev \
		sqlite-dev postgresql-dev hiredis-dev leveldb-dev \
		gmp-dev jsoncpp-dev ninja ca-certificates

WORKDIR /usr/src/
RUN git clone --recursive https://github.com/jupp0r/prometheus-cpp && \
		cd prometheus-cpp && \
		cmake -B build \
			-DCMAKE_INSTALL_PREFIX=/usr/local \
			-DCMAKE_BUILD_TYPE=Release \
			-DENABLE_TESTING=0 \
			-GNinja && \
		cmake --build build && \
		cmake --install build && \
	cd /usr/src/ && \
	git clone --recursive https://github.com/libspatialindex/libspatialindex && \
		cd libspatialindex && \
		cmake -B build \
			-DCMAKE_INSTALL_PREFIX=/usr/local && \
		cmake --build build && \
		cmake --install build && \
	cd /usr/src/ && \
	git clone --recursive https://luajit.org/git/luajit.git -b ${LUAJIT_VERSION} && \
		cd luajit && \
		make amalg && make install && \
	cd /usr/src/

FROM dev as builder

COPY .git /usr/src/opalclient/.git
COPY CMakeLists.txt /usr/src/opalclient/CMakeLists.txt
COPY README.md /usr/src/opalclient/README.md
COPY opalclient.conf.example /usr/src/opalclient/opalclient.conf.example
COPY builtin /usr/src/opalclient/builtin
COPY cmake /usr/src/opalclient/cmake
COPY doc /usr/src/opalclient/doc
COPY fonts /usr/src/opalclient/fonts
COPY lib /usr/src/opalclient/lib
COPY misc /usr/src/opalclient/misc
COPY po /usr/src/opalclient/po
COPY src /usr/src/opalclient/src
COPY irr /usr/src/opalclient/irr
COPY textures /usr/src/opalclient/textures

WORKDIR /usr/src/opalclient
RUN cmake -B build \
		-DCMAKE_INSTALL_PREFIX=/usr/local \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SERVER=TRUE \
		-DENABLE_PROMETHEUS=TRUE \
		-DBUILD_UNITTESTS=FALSE -DBUILD_BENCHMARKS=FALSE \
		-DBUILD_CLIENT=FALSE \
		-GNinja && \
	cmake --build build && \
	cmake --install build

FROM $DOCKER_IMAGE AS runtime

RUN apk add --no-cache curl gmp libstdc++ libgcc libpq jsoncpp zstd-libs \
				sqlite-libs postgresql hiredis leveldb && \
	adduser -D opalclient --uid 30000 -h /var/lib/opalclient && \
	chown -R opalclient:opalclient /var/lib/opalclient

WORKDIR /var/lib/opalclient

COPY --from=builder /usr/local/share/opalclient /usr/local/share/opalclient
COPY --from=builder /usr/local/bin/opalclientserver /usr/local/bin/opalclientserver
COPY --from=builder /usr/local/share/doc/opalclient/opalclient.conf.example /etc/opalclient/opalclient.conf
COPY --from=builder /usr/local/lib/libspatialindex* /usr/local/lib/
COPY --from=builder /usr/local/lib/libluajit* /usr/local/lib/
USER opalclient:opalclient

EXPOSE 30000/udp 30000/tcp
VOLUME /var/lib/opalclient/ /etc/opalclient/

ENTRYPOINT ["/usr/local/bin/opalclientserver"]
CMD ["--config", "/etc/opalclient/opalclient.conf"]
