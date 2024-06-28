#   Dockerfile for the RCLL Refbox
#   Created on Wed Jun 05 15:12:42 2019
#   Copyright (C) 2019 Till Hofmann <hofmann@kbsg.rwth-aachen.de>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#

FROM fedora:40 as builder
RUN   dnf update -y --refresh && dnf install -y --nodocs 'dnf-command(copr)' && \
      dnf -y copr enable thofmann/clips-6.31 && \
      dnf -y copr enable tavie/clips_protobuf && \
      dnf -y copr enable tavie/paho-mqtt-cpp && \
      dnf -y copr enable tavie/mongo-cxx-driver && \
      dnf install -y --nodocs \
      utf8proc-devel \
      avahi-devel \
      boost-devel \
      clips-devel \
      clipsmm-devel \
      freeopcua-devel \
      gcc-c++ \
      gecode-devel \
      git \
      glibmm24-devel \
      gtkmm30-devel \
      make \
      mongo-cxx-driver-devel \
      ncurses-devel \
      openssh-clients \
      openssl-devel \
      protobuf-compiler \
      protobuf-devel \
      protobuf_comm-devel \
      which \
      yaml-cpp-devel \
      libmicrohttpd-devel \
      libbson-devel \
      rapidjson-devel \
      apr-util-devel \
      gcc \
      cmake \
      paho-c-devel \
      paho-mqtt-cpp \
      paho-mqtt-cpp-devel \
      mongo-c-driver-devel \
    && \
    dnf install -y --nodocs rpm-build && \
    dnf clean all

FROM builder as buildenv
COPY . /buildenv/
SHELL ["/usr/bin/bash", "-c"]
WORKDIR /buildenv
RUN rm -rfd build
RUN mkdir -p build
# Set the working directory to the build directory
WORKDIR /buildenv/build

# Run CMake to configure the project
RUN cmake -DCONFDIR=/etc/rcll-refbox -DSHAREDIR=/usr/local/share/rcll-refbox - DBINDIR=/usr/local/bin ..

# Build the project
RUN make -j

# Run tests if needed (optional)
# RUN make test

# Install the built project
RUN make install -j

# RUN make -j`nproc` -l`nproc` USE_AVAHI=0 FAIL_ON_WARNING=1 \
#     EXEC_CONFDIR=/etc/rcll-refbox/ EXEC_BINDIR=/usr/local/bin EXEC_LIBDIR=/usr/local/lib64 \
#     EXEC_SHAREDIR=/usr/local/share/rcll-refbox
# Compute the dependencies and store them in requires.txt
WORKDIR /buildenv
RUN shopt -s globstar; \
    /usr/lib/rpm/rpmdeps -P /usr/local/lib64/** /usr/local/bin/** > provides.txt && \
    /usr/lib/rpm/rpmdeps -R /usr/local/lib64/** /usr/local/bin/** | grep -v -f provides.txt > requires.txt

FROM fedora:40 as refbox
RUN   dnf update -y --refresh && dnf install -y --nodocs 'dnf-command(copr)' && \
      dnf -y copr enable thofmann/clips-6.31 && \
      dnf -y copr enable tavie/clips_protobuf && \
      dnf -y copr enable tavie/paho-mqtt-cpp && \
      dnf -y copr enable tavie/mongo-cxx-driver && \
      dnf install -y --nodocs paho-c paho-c-devel paho-mqtt-cpp paho-mqtt-cpp-devel
COPY --from=buildenv /usr/local/bin /usr/local/bin/
COPY --from=buildenv /usr/local/lib64 /usr/local/lib64/
COPY --from=buildenv /usr/local/share /usr/local/share/
COPY --from=buildenv /etc/rcll-refbox /etc/rcll-refbox/
COPY --from=buildenv /buildenv/requires.txt /
RUN echo /usr/local/lib64 > /etc/ld.so.conf.d/local.conf && /sbin/ldconfig
RUN dnf install -y --nodocs $(cat /requires.txt) && dnf clean all && rm /requires.txt
ENV PATH="/usr/local/bin:${PATH}"
RUN mkdir /logs
WORKDIR logs
CMD ["refbox"]
