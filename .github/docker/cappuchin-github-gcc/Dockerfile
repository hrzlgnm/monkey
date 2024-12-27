FROM ghcr.io/void-linux/void-glibc
RUN xbps-install -Syu || xbps-install -yu xbps \
    && xbps-install -yu \
    && xbps-install -y bash git cmake ninja gcc libsanitizer-devel make \
    && xbps-remove -Ooy


