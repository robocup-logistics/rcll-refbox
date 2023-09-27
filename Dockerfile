#   Dockerfile for the RCLL Refbox
#   Created on Wed Jun 05 15:12:42 2019
#   Copyright (C) 2019 Till Hofmann <hofmann@kbsg.rwth-aachen.de>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#

FROM fedora:38 as builder
RUN   dnf update -y --refresh && dnf install -y --nodocs 'dnf-command(copr)' && \
      dnf -y copr enable thofmann/clips-6.31 && \
      dnf install -y --nodocs \
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
      which \
      yaml-cpp-devel \
      libmicrohttpd-devel \
      rapidjson-devel \
      apr-util-devel \
    && \
    dnf install -y --nodocs rpm-build && \
    dnf clean all

FROM builder as buildenv
COPY . /buildenv/
SHELL ["/usr/bin/bash", "-c"]
WORKDIR /buildenv
RUN make -j`nproc` -l`nproc` USE_AVAHI=0 FAIL_ON_WARNING=1 \
    EXEC_CONFDIR=/etc/rcll-refbox/ EXEC_BINDIR=/usr/local/bin EXEC_LIBDIR=/usr/local/lib64 \
    EXEC_SHAREDIR=/usr/local/share/rcll-refbox
# Compute the dependencies and store them in requires.txt
RUN shopt -s globstar; \
    /usr/lib/rpm/rpmdeps -P lib/** bin/** > provides.txt && \
    /usr/lib/rpm/rpmdeps -R lib/** bin/** | grep -v -f provides.txt > requires.txt

FROM fedora:38 as refbox
COPY --from=buildenv /buildenv/bin/* /usr/local/bin/
COPY --from=buildenv /buildenv/lib/* /usr/local/lib64/
COPY --from=buildenv /buildenv/src/games /usr/local/share/rcll-refbox/games
COPY --from=buildenv /buildenv/src/msgs/*.proto /usr/local/share/rcll-refbox/msgs/
COPY --from=buildenv /buildenv/src/libs/websocket/message_schemas/*.json /usr/local/share/rcll-refbox/libs/websocket/message_schemas/
COPY --from=buildenv /buildenv/cfg /etc/rcll-refbox/
COPY --from=buildenv /buildenv/requires.txt /
RUN echo /usr/local/lib64 > /etc/ld.so.conf.d/local.conf && /sbin/ldconfig
RUN dnf install -y --nodocs $(cat /requires.txt) && dnf clean all && rm /requires.txt
CMD ["llsf-refbox"]

FROM builder as devcontainer
ARG USER_NAME 
ENV USER_NAME=$USER_NAME
RUN useradd -u 1000 $USER_NAME
