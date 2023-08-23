FROM alpine:latest AS build

RUN apk add cargo protobuf protobuf-dev

WORKDIR /build
COPY src/ src/
COPY api/ api/
COPY Cargo.toml Cargo.lock build.rs ./

RUN cargo build --release

FROM alpine:latest AS runtime

RUN apk add libgcc

WORKDIR /app
COPY --from=build /build/target/release/ganymede /app/ganymede

EXPOSE 3000/tcp
ENTRYPOINT [ "/app/ganymede" ]