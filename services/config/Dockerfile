# Would be great to get this running on Alpine. Needs investigation
FROM ubuntu:20.10 AS runtime

COPY bin/ganymede.services.config /opt/ganymede/ganymede.services.config

ENV PORT=3000
EXPOSE ${PORT}
ENTRYPOINT ["/opt/ganymede/ganymede.services.config"]