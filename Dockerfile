# Dockerfile
# Copyright (C) 2019 Till Hofmann <hofmann@kbsg.rwth-aachen.de>
#
# Distributed under terms of the MIT license.
#
FROM fedora:29 as buildenv
RUN dnf install -y --nodocs \
      avahi-devel \
      boost-devel \
      clips-devel \
      clipsmm-devel \
      gcc-c++ \
      gecode-devel \
      git \
      glibmm24-devel \
      gtkmm30-devel \
      mongodb-devel \
      ncurses-devel \
      openssh-clients \
      openssl-devel \
      protobuf-compiler \
      protobuf-devel \
      which \
      yaml-cpp-devel \
    && \
    dnf install -y --nodocs 'dnf-command(copr)' && \
    dnf -y copr enable thofmann/freeopcua && \
    dnf install -y --nodocs freeopcua-devel && \
    dnf install -y --nodocs rpm-build && \
    dnf clean all
COPY . /buildenv/
SHELL ["/usr/bin/bash", "-c"]
WORKDIR /buildenv
RUN make -j`nproc` -l`nproc` FAIL_ON_WARNING=1 \
    EXEC_CONFDIR=/etc/rcll-refbox EXEC_BINDIR=/usr/local/bin EXEC_LIBDIR=/usr/local/lib64 \
    EXEC_SHAREDIR=/usr/local/share/rcll-refbox
# Compute the dependencies and store them in requires.txt
RUN shopt -s globstar; \
    /usr/lib/rpm/rpmdeps -P lib/** bin/** > provides.txt && \
    /usr/lib/rpm/rpmdeps -R lib/** bin/** | grep -v -f provides.txt > requires.txt

FROM fedora:29
COPY --from=buildenv /buildenv/bin/* /usr/local/bin/
COPY --from=buildenv /buildenv/lib/* /usr/local/lib64/
COPY --from=buildenv /buildenv/src/games /usr/local/share/rcll-refbox/games
COPY --from=buildenv /buildenv/src/msgs /usr/local/share/rcll-refbox/msgs
COPY --from=buildenv /buildenv/cfg/* /etc/rcll-refbox/
COPY --from=buildenv /buildenv/requires.txt /
RUN echo /usr/local/lib64 > /etc/ld.so.conf.d/local.conf && /sbin/ldconfig
RUN dnf install -y --nodocs 'dnf-command(copr)' && \
    dnf -y copr enable thofmann/freeopcua && \
    dnf install -y --nodocs $(cat /requires.txt) && dnf clean all && rm /requires.txt
CMD ["llsf-refbox"]
