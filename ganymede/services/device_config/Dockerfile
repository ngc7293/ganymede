FROM ubuntu:focal AS base

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update
RUN apt-get install g++ cmake ninja-build python3 python3-pip -y
RUN pip install conan

FROM base AS build

# Third parties
ENV CONAN_USER_HOME=/var/build/
COPY conanfile.txt  /var/build/
WORKDIR /var/build/cmake/conan
RUN conan install ../.. -s compiler.libcxx=libstdc++11 -s cppstd=17 -s build_type=Release

# Copy project source
COPY CMakeLists.txt /var/build/
COPY ganymede       /var/build/ganymede

# Build project
WORKDIR /var/build/cmake/build
RUN cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ../..
RUN ninja ganymede.services.device_config

FROM ubuntu:focal AS runtime

RUN apt-get update
RUN apt-get -y install ca-certificates

COPY --from=build /var/build/cmake/build/bin/ /opt/bin/
COPY ganymede/services/device_config/device_config.service_config.json /opt/var/

ENV PORT=3000

EXPOSE ${PORT}
WORKDIR /opt/
ENTRYPOINT ["./bin/ganymede.services.device_config", "./var/device_config.service_config.json"]