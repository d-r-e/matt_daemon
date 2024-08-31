FROM alpine:latest
RUN apk --update add \
    busybox-extras \
    clang \
    gcc \
    git \
    make \
    psutils \
    tzdata \
    valgrind \
    iproute2 \
    iputils \
    && rm -rf /var/cache/apk/*
ENV TZ=Europe/Madrid
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone