FROM debian:stretch

USER root
RUN apt-get update -y && \
	apt-get -y install build-essential libirrlicht-dev cmake libbz2-dev libpng-dev libjpeg-dev \
		libsqlite3-dev libcurl4-gnutls-dev zlib1g-dev libgmp-dev libjsoncpp-dev git

COPY . /usr/src/minetest

RUN	mkdir -p /usr/src/minetest/cmakebuild && cd /usr/src/minetest/cmakebuild && \
    	cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release -DRUN_IN_PLACE=FALSE \
		-DBUILD_SERVER=TRUE \
		-DBUILD_CLIENT=FALSE \
		-DENABLE_SYSTEM_JSONCPP=1 \
		.. && \
		make -j2 && \
		rm -Rf ../games/minetest_game && git clone --depth 1 https://github.com/minetest/minetest_game ../games/minetest_game && \
		rm -Rf ../games/minetest_game/.git && \
		make install

FROM debian:stretch

USER root
RUN groupadd minetest && useradd -m -g minetest -d /var/lib/minetest minetest && \
    apt-get update -y && \
    apt-get -y install libcurl3-gnutls libjsoncpp1 liblua5.1-0 libluajit-5.1-2 libpq5 libsqlite3-0 \
        libstdc++6 zlib1g libc6 && \
    apt-get clean && rm -rf /var/cache/apt/archives/* && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /var/lib/minetest

COPY --from=0 /usr/local/share/minetest /usr/local/share/minetest
COPY --from=0 /usr/local/bin/minetestserver /usr/local/bin/minetestserver
COPY --from=0 /usr/local/share/doc/minetest/minetest.conf.example /etc/minetest/minetest.conf

USER minetest

EXPOSE 30000/udp

CMD ["/usr/local/bin/minetestserver", "--config", "/etc/minetest/minetest.conf"]
