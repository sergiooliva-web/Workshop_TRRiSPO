FROM ubuntu:24.04

COPY dist/crypto-tool_*_amd64.deb /tmp/app.deb
RUN apt-get update \
    && apt-get install -y --no-install-recommends /tmp/app.deb curl \
    && rm -rf /var/lib/apt/lists/* /tmp/app.deb

USER 10001
ENV APP_PORT=8080
EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=3s \
    CMD curl -fs http://localhost:8080/health || exit 1

CMD ["crypto_tool", "serve"]
