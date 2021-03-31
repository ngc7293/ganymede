FROM alpine:latest AS base

RUN apk update
RUN apk upgrade
RUN apk add git g++ cmake ninja g++ openssh openssl-dev linux-headers python3

FROM base AS build

# Copy only third-party files. We want to copy the smallest amount of files so
# that it invalidates Docker's cache as seldomly as possible.
RUN mkdir /var/build /opt/local
COPY .git                   /var/build/.git
COPY .gitignore .gitmodules /var/build/
COPY third_party/build.py   /var/build/third_party/

# Build third parties
WORKDIR /var/build/third_party
RUN git submodule update --init --recursive
RUN ./build.py --project=grpc,json,cpp-jwt --prefix=/opt/local --release

# Copy project source
COPY CMakeLists.txt /var/build/
COPY ganymede       /var/build/ganymede

# Build project
ENV CMAKE_PREFIX_PATH=/opt/local
ENV GANYMEDE_SCOPE="service.measurements"
WORKDIR /var/build/cmake/build
RUN cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/local ../..
RUN ninja ganymede.services.measurements
RUN ninja install

FROM alpine:latest AS runtime

COPY --from=build /usr/local/lib /usr/local/lib
COPY --from=build /usr/local/lib64 /usr/local/lib64
RUN apk update
RUN apk upgrade
RUN apk add libstdc++
COPY --from=build /var/build/cmake/build/bin/ /opt/bin/
COPY ganymede/services/measurements/measurements.service_config.json /opt/var/

RUN echo "/lib:/usr/lib:/usr/local/lib:/usr/local/lib64" > /etc/ld-musl-x86_64.path

ENV PORT=3000

EXPOSE ${PORT}
WORKDIR /opt/
ENTRYPOINT ["./bin/ganymede.services.measurements", "./var/measurements.service_config.json"]