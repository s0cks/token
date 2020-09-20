FROM token-ledger-base:1.1.0
VOLUME /usr/share/token/ledger
RUN mkdir -p /usr/src/token-node/
COPY entrypoint.sh /usr/src/token-node/
RUN chmod +x /usr/src/token-node/entrypoint.sh
ENTRYPOINT [ "/usr/src/token-node/entrypoint.sh" ]