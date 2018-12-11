FROM debian:stretch

COPY . /usr/src/minetest

USER root
RUN apt-get update -y && \
	apt-get -y install build-essential libirrlicht-dev cmake libbz2-dev libpng-dev libjpeg-dev \
		libsqlite3-dev libcurl4-gnutls-dev zlib1g-dev libgmp-dev libjsoncpp-dev

RUN	mkdir /usr/src/minetest/cmakebuild && cd /usr/src/minetest/cmakebuild && \
    	cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release -DRUN_IN_PLACE=FALSE \
		-DENABLE_GETTEXT=TRUE \
		-DBUILD_SERVER=TRUE \
		-DBUILD_CLIENT=FALSE \
		-DENABLE_SOUND=0 .. && \
	make -j2 && make install

USER root
RUN	apt-get clean && rm -rf /var/cache/apt/archives/* && \
	rm -rf /var/lib/apt/lists/*
