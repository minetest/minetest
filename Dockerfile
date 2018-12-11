FROM debian:stretch

COPY . /usr/src/minetest

USER root
RUN apt-get update -y && \
	apt-get -y install build-essential libirrlicht-dev cmake libbz2-dev libpng-dev libjpeg-dev \
		libsqlite3-dev libcurl4-gnutls-dev zlib1g-dev libgmp-dev libjsoncpp-dev && \
		useradd -m -d /usr/src/minetest builder

USER builder
RUN	mkdir /usr/src/minetest/cmakebuild && cd /usr/src/minetest/cmakebuild && \
    	cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release -DRUN_IN_PLACE=FALSE \
		-DENABLE_GETTEXT=TRUE \
		-DBUILD_SERVER=TRUE \
		-DBUILD_CLIENT=FALSE \
		-DENABLE_SYSTEM_JSONCPP=1 \
		-DENABLE_SOUND=0 .. && \
	make -j2 && make install

USER root
RUN	apt-get clean && rm -rf /var/cache/apt/archives/* && \
	rm -rf /var/lib/apt/lists/*

FROM debian:stretch

USER root
RUN groupadd minetest && useradd -m -g -d /var/lib/minetest minetest &&
    apt-get update -y && \
    apt-get -y install libcurl3-gnutls libjsoncpp1 liblua5.1-0 libluajit-5.1-2 libpq5 libsqlite3-0 \
        libstdc++6 zlib1g

WORKDIR /var/lib/minetest

COPY --from=0 /usr/local/share/minetest /usr/local/share/minetest
COPY --from=0 /usr/local/bin/minetestserver /usr/local/bin/minetestserver

RUN find /usr/local
