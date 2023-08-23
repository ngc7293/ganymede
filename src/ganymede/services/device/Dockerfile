FROM ghcr.io/ngc7293/cxxci:jammy AS build

# Third parties
ENV CONAN_USER_HOME=/var/build/
COPY conanfile.txt  /var/build/
WORKDIR /var/build/build

RUN conan config set general.revisions_enabled=1
RUN conan remote add davidbourgault https://artifactory.davidbourgault.ca/artifactory/api/conan/conan
RUN conan install .. --settings compiler.libcxx=libstdc++11 --settings build_type=Release --remote davidbourgault

# # Copy project source
COPY CMakeLists.txt /var/build/
COPY libs           /var/build/libs
COPY src            /var/build/src

# Build project
WORKDIR /var/build/build
RUN cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
RUN ninja ganymede.services.device

FROM ubuntu:latest AS runtime

RUN apt-get update
RUN apt-get -y install ca-certificates

COPY --from=build /var/build/build/bin/ /opt/bin/
COPY src/ganymede/services/device/device.config.json /opt/var/

ENV PORT=3000

EXPOSE ${PORT}
WORKDIR /opt/
ENTRYPOINT ["./bin/ganymede.services.device", "./var/device.config.json"]