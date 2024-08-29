FROM alpine:latest
RUN apk --update add \
    gcc \
    clang \
    make \
    valgrind \
    git \
    psutils \
    && rm -rf /var/cache/apk/*