ARG DOCKER_IMAGE=alpine:3.16
FROM $DOCKER_IMAGE AS builder

ENV MINETEST_GAME_VERSION master
ENV IRRLICHT_VERSION master
ENV SPATIALINDEX_VERSION 1.9.3
ENV LUAJIT_VERSION v2.1

RUN apk add --no-cache git build-base cmake curl-dev zlib-dev zstd-dev \
		sqlite-dev postgresql-dev hiredis-dev leveldb-dev \
		gmp-dev jsoncpp-dev linux-headers ninja ca-certificates

WORKDIR /usr/src/
RUN git clone --recursive https://github.com/jupp0r/prometheus-cpp/ && \
		cd prometheus-cpp && \
		cmake -B build \
			-DCMAKE_INSTALL_PREFIX=/usr/local \
			-DCMAKE_BUILD_TYPE=Release \
			-DENABLE_TESTING=0 \
			-GNinja && \
		cmake --build build && \
		cmake --install build && \
	cd /usr/src/ && \
	git clone --recursive https://github.com/libspatialindex/libspatialindex -b ${SPATIALINDEX_VERSION} && \
		cd libspatialindex && \
		cmake -B build \
			-DCMAKE_INSTALL_PREFIX=/usr/local && \
		cmake --build build && \
		cmake --install build && \
	cd /usr/src/ && \
	git clone --recursive https://luajit.org/git/luajit.git -b ${LUAJIT_VERSION} && \
		cd luajit && \
		make && make install && \
	cd /usr/src/ && \
	git clone --depth=1 https://github.com/minetest/irrlicht/ -b ${IRRLICHT_VERSION} && \
		cp -r irrlicht/include /usr/include/irrlichtmt

COPY .git /usr/src/minetest/.git
COPY CMakeLists.txt /usr/src/minetest/CMakeLists.txt
COPY README.md /usr/src/minetest/README.md
COPY minetest.conf.example /usr/src/minetest/minetest.conf.example
COPY builtin /usr/src/minetest/builtin
COPY cmake /usr/src/minetest/cmake
COPY doc /usr/src/minetest/doc
COPY fonts /usr/src/minetest/fonts
COPY lib /usr/src/minetest/lib
COPY misc /usr/src/minetest/misc
COPY po /usr/src/minetest/po
COPY src /usr/src/minetest/src
COPY textures /usr/src/minetest/textures

WORKDIR /usr/src/minetest
RUN git clone --depth=1 -b ${MINETEST_GAME_VERSION} https://github.com/minetest/minetest_game.git ./games/minetest_game && \
		rm -fr ./games/minetest_game/.git && \
		cmake -B build \
			-DCMAKE_INSTALL_PREFIX=/usr/local \
			-DCMAKE_BUILD_TYPE=Release \
			-DBUILD_SERVER=TRUE \
			-DENABLE_PROMETHEUS=TRUE \
			-DBUILD_UNITTESTS=FALSE \
			-DBUILD_CLIENT=FALSE \
			-GNinja && \
		cmake --build build && \
		cmake --install build

ARG DOCKER_IMAGE=alpine:3.16
FROM $DOCKER_IMAGE AS runtime

RUN apk add --no-cache curl gmp libstdc++ libgcc libpq jsoncpp zstd-libs \
				sqlite-libs postgresql hiredis leveldb && \
	adduser -D minetest --uid 30000 -h /var/lib/minetest && \
	chown -R minetest:minetest /var/lib/minetest

WORKDIR /var/lib/minetest

COPY --from=builder /usr/local/share/minetest /usr/local/share/minetest
COPY --from=builder /usr/local/bin/minetestserver /usr/local/bin/minetestserver
COPY --from=builder /usr/local/share/doc/minetest/minetest.conf.example /etc/minetest/minetest.conf
COPY --from=builder /usr/local/lib/libspatialindex* /usr/local/lib/
COPY --from=builder /usr/local/lib/libluajit* /usr/local/lib/
USER minetest:minetest

EXPOSE 30000/udp 30000/tcp
VOLUME /var/lib/minetest/ /etc/minetest/

ENTRYPOINT ["/usr/local/bin/minetestserver"]
CMD ["--config", "/etc/minetest/minetest.conf"]
