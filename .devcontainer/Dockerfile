FROM alpine:latest
RUN apk --update add \
    gcc \
    clang \
    make \
    valgrind \
    git \
    psutils \
    tzdata \
    && rm -rf /var/cache/apk/*
ENV TZ=Europe/Madrid
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone