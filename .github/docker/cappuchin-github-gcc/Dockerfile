FROM ghcr.io/void-linux/void-musl
RUN xbps-install -Syu || xbps-install -yu xbps \
    && xbps-install -yu \
    && xbps-install -y bash git cmake ninja gcc \
    && xbps-remove -Ooy


