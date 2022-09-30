#   Dockerfile for the RCLL Refbox
#   Created on Wed Jun 05 15:12:42 2019
#   Copyright (C) 2019 Till Hofmann <hofmann@kbsg.rwth-aachen.de>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#

FROM fedora:35
RUN   dnf install -y --nodocs 'dnf-command(copr)' && \
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
      cmake \
      open62541-devel \
      net-tools \
      iputils \
    && \
    dnf install -y --nodocs rpm-build && \
    dnf clean all
ARG USER_NAME 
ENV USER_NAME=$USER_NAME
RUN useradd -u 1000 $USER_NAME
#COPY /etc/group /etc/group 
#COPY /etc/passwd /etc/passwd
#COPY /etc/shadow /etc/shadow

#RUN git clone git@github.com:open62541/open62541.git
#RUN cd open62541 && mkdir build && cd build
#RUN cmake ..
#RUN install

# trying a new OPCUA client software
# RUN dnf -y install open62541