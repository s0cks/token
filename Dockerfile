FROM token-ledger-base:1.5.2
# Data
EXPOSE 8080
# Health Check
EXPOSE 8081


VOLUME /usr/share/token/ledger
RUN mkdir -p /usr/src/token-node/
COPY entrypoint.sh /usr/src/token-node/
RUN chmod +x /usr/src/token-node/entrypoint.sh
ENTRYPOINT [ "/usr/src/token-node/entrypoint.sh" ]