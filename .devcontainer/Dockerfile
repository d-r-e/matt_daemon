FROM alpine:3.20
RUN apk --update add \
    bash \
    busybox-extras \
    clang \
    gcc \
    git \
    iproute2 \
    iputils \
    make \
    psutils \
    tzdata \
    valgrind \
    && rm -rf /var/cache/apk/*
ENV TZ=Europe/Madrid
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone