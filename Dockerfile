FROM token-ledger-base:1.2.1
VOLUME /usr/share/token/ledger
RUN mkdir -p /usr/src/token-node/
COPY entrypoint.sh /usr/src/token-node/
RUN chmod +x /usr/src/token-node/entrypoint.sh
EXPOSE 8080
ENTRYPOINT [ "/usr/src/token-node/entrypoint.sh" ]