FROM token-ledger-base:1.0.0
ARG GITHUB_TOKEN

# Copy The Ledger Source
COPY CMakeLists.txt main.cc tests.cc inspector.cc entrypoint.sh /usr/src/libtoken-ledger/
COPY cmake /usr/src/libtoken-ledger/cmake/
COPY include /usr/src/libtoken-ledger/include/
COPY src /usr/src/libtoken-ledger/src/
COPY tests /usr/src/libtoken-ledger/tests/
RUN mkdir -p /usr/src/libtoken-ledger/build

WORKDIR /usr/src/libtoken-ledger
RUN cd build/ \
 && cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr/local/ ..\
 && cmake --build . --target install \
 && ldconfig
RUN chmod +x ./entrypoint.sh

USER root
ENTRYPOINT [ "/usr/src/libtoken-ledger/entrypoint.sh" ]

# Expose the REST Service
EXPOSE 8080
# Expose the RPC Service
EXPOSE 8081
# Expose the HealthCheck Service
EXPOSE 8082