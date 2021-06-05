FROM alpine:latest AS base

RUN apk add git g++ cmake ninja g++ openssh openssl-dev linux-headers python3 python3-pip
RUN pip install conan

FROM base AS build

# Copy only third-party files. We want to copy the smallest amount of files so
# that it invalidates Docker's cache as seldomly as possible.
# Build third parties

# Copy project source
COPY CMakeLists.txt /var/build/
COPY conanfile.txt  /var/build/
COPY ganymede       /var/build/ganymede

# Build project
ENV CMAKE_PREFIX_PATH=/opt/local
ENV GANYMEDE_SCOPE="service.device_config"
WORKDIR /var/build/cmake/build
RUN conan install ../..
RUN cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ../..
RUN ninja ganymede.services.device_config

FROM alpine:latest AS runtime

RUN apk add libstdc++
COPY --from=build /var/build/cmake/build/bin/ /opt/bin/
COPY ganymede/services/device_config/device_config.service_config.json /opt/var/

RUN echo "/lib:/usr/lib:/usr/local/lib:/usr/local/lib64" > /etc/ld-musl-x86_64.path

ENV PORT=3000

EXPOSE ${PORT}
WORKDIR /opt/
ENTRYPOINT ["./bin/ganymede.services.device_config", "./var/device_config.service_config.json"]