FROM ghcr.io/ngc7293/cxxci:latest AS build

# Third parties
ENV CONAN_USER_HOME=/var/build/
COPY conanfile.txt  /var/build/
WORKDIR /var/build/build
RUN conan install .. -s compiler.libcxx=libstdc++11 -s cppstd=20 -s build_type=Release

# Copy project source
COPY CMakeLists.txt /var/build/
COPY libs           /var/build/libs
COPY ganymede       /var/build/ganymede

# Build project
WORKDIR /var/build/build
RUN cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
RUN ninja ganymede.services.measurements

FROM ubuntu:latest AS runtime

RUN apt-get update
RUN apt-get -y install ca-certificates

COPY --from=build /var/build/build/bin/ /opt/bin/
COPY ganymede/services/measurements/measurements.service_config.json /opt/var/

ENV PORT=3000

EXPOSE ${PORT}
WORKDIR /opt/
ENTRYPOINT ["./bin/ganymede.services.measurements", "./var/measurements.service_config.json"]