FROM builder as devcontainer
ARG USER_NAME
ENV USER_NAME=$USER_NAME
RUN useradd -u 1000 $USER_NAME
