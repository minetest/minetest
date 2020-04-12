FROM alpine

COPY . /usr/src/minetest

RUN apk add --no-cache git build-base irrlicht-dev cmake bzip2-dev libpng-dev \
		jpeg-dev libxxf86vm-dev mesa-dev sqlite-dev libogg-dev \
		libvorbis-dev openal-soft-dev curl-dev freetype-dev zlib-dev \
		gmp-dev jsoncpp-dev && \
	cd /usr/src/minetest && \
	git clone --depth=1 https://github.com/minetest/minetest_game.git ./games/minetest_game && \
	rm -fr ./games/minetest_game/.git && \
	cmake . \
		-DCMAKE_INSTALL_PREFIX=/usr/local \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SERVER=TRUE \
		-DBUILD_CLIENT=FALSE && \
	make -j2 && \
	make install

FROM alpine

RUN apk add --no-cache sqlite-libs curl gmp libstdc++ libgcc && \
	adduser -D minetest -h /var/lib/minetest

WORKDIR /var/lib/minetest

COPY --from=0 /usr/local/share/minetest /usr/local/share/minetest
COPY --from=0 /usr/local/bin/minetestserver /usr/local/bin/minetestserver
COPY --from=0 /usr/local/share/doc/minetest/minetest.conf.example /etc/minetest/minetest.conf

USER minetest

EXPOSE 30000/udp

CMD ["/usr/local/bin/minetestserver", "--config", "/etc/minetest/minetest.conf"]
