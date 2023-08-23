# Ganymede

Web service(s) to manage automated hydroponics systems.

This started as a capstone project for my bachelors project, which I maintained
for my personal use. While the initial version was implemented as multiple micro
services implemented in C++, it was later reimplemented as a monolith in Rust.

## Building

This readme used to have a long list of steps to help you get compilation
running, but now there's only two steps.

 - Make sure you have protoc in your `$PATH` (On Debians: `sudo apt install protobuf-compiler`)
 - `cargo build`

Thanks Rust :).

## Deploying

Files necessary for a full deployment are included in the `kubernetes/`
subfolder. They are purposefully kept simple, and are designed with MicroK8s
small-scale deployments in mind.

The service expects requests to include a JWT from Auth0. For further details,
and to update the claims name if needed, see `src/auth.rs`.