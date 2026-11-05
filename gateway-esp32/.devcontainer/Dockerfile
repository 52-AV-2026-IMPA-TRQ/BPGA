FROM node:22-bookworm-slim AS node_runtime

FROM espressif/idf:v6.0.2

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

# Reuse the official Node.js runtime and Corepack without altering ESP-IDF.
COPY --from=node_runtime /usr/local/ /usr/local/

RUN apt-get update -y \
    && apt-get install -y --no-install-recommends udev \
    && rm -rf /var/lib/apt/lists/* \
    && corepack enable pnpm

RUN echo "source /opt/esp/idf/export.sh > /dev/null 2>&1" >> ~/.bashrc

ENTRYPOINT ["/opt/esp/entrypoint.sh"]
CMD ["/bin/bash", "-c"]
