FROM ghcr.io/void-linux/void-musl
RUN xbps-install -Syu || xbps-install -yu xbps \
    && xbps-install -yu \
    && xbps-install -y bash git cmake clang python3-codespell ninja clang clang-tools-extra cppcheck \
    && xbps-remove -Oy


